import SwiftUI
import CoreBluetooth
import Combine

// MARK: - BLEManager

class BLEManager: NSObject, ObservableObject, CBCentralManagerDelegate, CBPeripheralDelegate {
  // MARK: - Properties

  private let serviceUUID = CBUUID(string: "FFE0")
  private let characteristicUUID = CBUUID(string: "FFE1")

  private var centralManager: CBCentralManager!
  private var peripheral: CBPeripheral?
  private var writeCharacteristic: CBCharacteristic?

  @Published var isConnected = false
  @Published var inboundCurrentThrottle: Double = 0
  @Published var inboundMaxThrottle: Double = 0

  override init() {
    super.init()
    centralManager = CBCentralManager(delegate: self, queue: nil)
  }

  // MARK: - Public Methods

  func connect() {
    // Start scanning for peripherals with the given service UUID
    centralManager.scanForPeripherals(withServices: [serviceUUID], options: nil)
  }

  /// Sends data as a simple CSV string: "maxThrottle,currentThrottle\n"
  func sendData(maxThrottle: Int, currentThrottle: Int) {
    guard isConnected,
          let peripheral = peripheral,
          let writeCharacteristic = writeCharacteristic
    else { return }

    let message = "\(maxThrottle),\(currentThrottle)\n"
    let messageData = Data(message.utf8)

    peripheral.writeValue(messageData, for: writeCharacteristic, type: .withResponse)
  }

  // MARK: - CBCentralManagerDelegate

  func centralManagerDidUpdateState(_ central: CBCentralManager) {
    guard central.state == .poweredOn else {
      print("Bluetooth is not available or powered off.")
      return
    }
    centralManager.scanForPeripherals(withServices: [serviceUUID], options: nil)
  }

  func centralManager(_ central: CBCentralManager,
                      didDiscover peripheral: CBPeripheral,
                      advertisementData: [String: Any],
                      rssi RSSI: NSNumber) {
    centralManager.stopScan()
    self.peripheral = peripheral
    self.peripheral?.delegate = self
    centralManager.connect(peripheral, options: nil)
  }

  func centralManager(_ central: CBCentralManager,
                      didConnect peripheral: CBPeripheral) {
    isConnected = true
    print("Connected to peripheral: \(peripheral.name ?? "Unknown")")

    let mtu = peripheral.maximumWriteValueLength(for: .withoutResponse)
    print("MTU is:", mtu)

    peripheral.discoverServices([serviceUUID])
  }

  func centralManager(_ central: CBCentralManager,
                      didDisconnectPeripheral peripheral: CBPeripheral,
                      timestamp: CFAbsoluteTime,
                      isReconnecting: Bool,
                      error: (any Error)?) {
    isConnected = false
    print("Disconnected from peripheral: \(peripheral.name ?? "Unknown")")
  }

  // MARK: - CBPeripheralDelegate

  func peripheral(_ peripheral: CBPeripheral,
                  didDiscoverServices error: Error?) {
    if let error = error {
      print("Error discovering services: \(error.localizedDescription)")
      return
    }
    guard let services = peripheral.services else { return }

    for service in services where service.uuid == serviceUUID {
      peripheral.discoverCharacteristics([characteristicUUID], for: service)
    }
  }

  func peripheral(_ peripheral: CBPeripheral,
                  didDiscoverCharacteristicsFor service: CBService,
                  error: Error?) {
    if let error = error {
      print("Error discovering characteristics: \(error.localizedDescription)")
      return
    }
    guard let characteristics = service.characteristics else { return }

    for characteristic in characteristics where characteristic.uuid == characteristicUUID {
      print("Found characteristic \(characteristic.uuid). Subscribing for notifications...")
      writeCharacteristic = characteristic
      peripheral.setNotifyValue(true, for: characteristic)
    }
  }

  func peripheral(_ peripheral: CBPeripheral,
                  didUpdateValueFor characteristic: CBCharacteristic,
                  error: Error?) {
    if let error = error {
      print("Error updating value: \(error.localizedDescription)")
      return
    }
    guard let data = characteristic.value else { return }

    // Handle a comma-delimited string in the format: "maxThrottle,currentThrottle"
    let csvString = String(decoding: data, as: UTF8.self).trimmingCharacters(in: .whitespacesAndNewlines)
    let parts = csvString.split(separator: ",")
    guard parts.count == 2 else {
      print("Invalid CSV. Expected two parts, got:", csvString)
      return
    }

    if let newMax = Double(parts[0]), let newCurrent = Double(parts[1]) {
      DispatchQueue.main.async {
        self.inboundMaxThrottle = newMax
        self.inboundCurrentThrottle = newCurrent
      }
    }
  }
}

// MARK: - ContentView

struct ContentView: View {
  @StateObject private var bleManager = BLEManager()

  @State private var maxThrottle: Double = 50
  @State private var currentThrottle: Double = 0

  // Track if the user is currently moving the slider
  @State private var isUserEditingThrottle = false

  // MARK: Debounce Publishers
  @State private var maxThrottleSubject = PassthroughSubject<Double, Never>()
  @State private var currentThrottleSubject = PassthroughSubject<Double, Never>()

  // Keep references to our Combine subscriptions
  @State private var cancellables: Set<AnyCancellable> = []

  var body: some View {
    VStack(spacing: 20) {
      if bleManager.isConnected {
        connectedView
      } else {
        notConnectedView
      }
    }
    .padding()
    // When inbound throttle changes (from the BLE device)
    .onChange(of: bleManager.inboundCurrentThrottle) { newValue in
      // Only update the UI slider if we are not currently editing
      if !isUserEditingThrottle {
        currentThrottle = newValue
      }
    }
    // When inbound max changes (from the BLE device)
    .onChange(of: bleManager.inboundMaxThrottle) { newValue in
      maxThrottle = newValue
    }
    // Setup our Combine pipelines once
    .onAppear {
      setupDebouncePipelines()
    }
  }

  // MARK: - Subviews

  private var notConnectedView: some View {
    VStack(spacing: 16) {
      ProgressView(
        label: { Text("Connecting …") }
      ).scaleEffect(2.0)
    }
  }

  private var connectedView: some View {
    VStack(alignment: .leading, spacing: 20) {
      Text("Motorcycle Connected!").font(.title)
      throttleMaxSlider
      currentThrottleSlider
    }
    .padding()
  }

  private var throttleMaxSlider: some View {
    VStack(alignment: .leading, spacing: 8) {
      Text("Max Throttle: \(Int(maxThrottle))")
      Slider(
        value: $maxThrottle,
        in: 0...100,
        step: 1
      )
      .onChange(of: maxThrottle) { newValue in
        // Fire an event into our Subject, which then gets debounced
        maxThrottleSubject.send(newValue)
      }
    }
  }

  private var currentThrottleSlider: some View {
    VStack(alignment: .leading, spacing: 8) {
      Text("Current Throttle: \(Int(currentThrottle))")
      Slider(
        value: $currentThrottle,
        in: 0...100,
        step: 1,
        onEditingChanged: { isEditing in
          // Called when user starts (true) or ends (false) dragging
          isUserEditingThrottle = isEditing
          if !isEditing {
            // The moment user lifts their finger, reset to 0
            currentThrottle = 0
            bleManager.sendData(
              maxThrottle: Int(maxThrottle),
              currentThrottle: -1
            )
          }
        }
      )
      .onChange(of: currentThrottle) { newValue in
        // Fire an event into our Subject, which then gets debounced
        currentThrottleSubject.send(newValue == 0 ? -1 : newValue)
      }
    }
  }

  // MARK: - Throttle Setup

  private func setupDebouncePipelines() {
    // Throttle changes to maxThrottle by 50 ms
    maxThrottleSubject
      .throttle(for: .milliseconds(100), scheduler: RunLoop.main, latest: true)
      .sink { newValue in
        bleManager.sendData(
          maxThrottle: Int(newValue),
          currentThrottle: Int(currentThrottle == 0 ? -1 : currentThrottle)
        )
      }
      .store(in: &cancellables)

    // Throttle changes to currentThrottle by 50 ms
    currentThrottleSubject
      .throttle(for: .milliseconds(100), scheduler: RunLoop.main, latest: true)
      .sink { newValue in
        // Only send if we're not forcibly resetting to 0.
        if currentThrottle > 0 && isUserEditingThrottle {
          bleManager.sendData(
            maxThrottle: Int(maxThrottle),
            currentThrottle: Int(newValue)
          )
        }
      }
      .store(in: &cancellables)
  }
}
