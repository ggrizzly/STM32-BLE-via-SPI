//
//  ViewController.swift
//  TrailCounterApp
//
//  Created by Jordan Ventura Gaeta on 4/6/16.
//  Copyright © 2016 p442. All rights reserved.
//

import UIKit
import CoreBluetooth

class ViewController: UIViewController, CBCentralManagerDelegate, CBPeripheralDelegate, UITableViewDelegate, UITableViewDataSource {
    
    lazy var refreshControl: UIRefreshControl = {
        let refreshControl = UIRefreshControl()
        refreshControl.addTarget(self, action: "handleRefresh:", forControlEvents: UIControlEvents.ValueChanged)
        
        return refreshControl
    }()
    
    var peripherals: Set<CBPeripheral> = Set<CBPeripheral>()
    var currentPeripheral: CBPeripheral?
    
    var titleLabel : UILabel!
    var statusLabel : UILabel!
    var tableLabel : UILabel!
    var tableView: UITableView  =   UITableView()
    //var shouldConnect = false
    
    // BLE
    var centralManager : CBCentralManager!
    var blePeripheral : CBPeripheral!
    
    // IR Temp UUIDs
    let RXDataUUID = CBUUID(string: "6E400003-B5A3-F393-E0A9-E50E24DCCA9E")
    let TXDataUUID   = CBUUID(string: "6E400002-B5A3-F393-E0A9-E50E24DCCA9E")
    let UARTServiceUUID = CBUUID(string: "6E400001-B5A3-F393-E0A9-E50E24DCCA9E")
    let deviceName = "Adafruit Bluefruit LE"

    override func viewDidLoad() {
        super.viewDidLoad()
        
        // Set up title label
        titleLabel = UILabel()
        titleLabel.text = "BLE TAG"
        titleLabel.font = UIFont(name: "HelveticaNeue-Bold", size: 20)
        titleLabel.sizeToFit()
        titleLabel.center = CGPoint(x: self.view.frame.midX, y: self.titleLabel.bounds.midY+28)
        self.view.addSubview(titleLabel)
        
        // Set up status label
        statusLabel = UILabel()
        statusLabel.textAlignment = NSTextAlignment.Center
        statusLabel.text = "Loading..."
        statusLabel.font = UIFont(name: "HelveticaNeue-Light", size: 12)
        statusLabel.sizeToFit()
        statusLabel.frame = CGRect(x: self.view.frame.origin.x, y: self.titleLabel.frame.maxY, width: self.view.frame.width, height: self.statusLabel.bounds.height)
        self.view.addSubview(statusLabel)
        
        tableView.frame = CGRectMake(0, 100, 420, 500);
        tableView.delegate = self
        tableView.dataSource = self
        
        tableView.registerClass(UITableViewCell.self, forCellReuseIdentifier: "cell")
        
        self.tableView.addSubview(self.refreshControl)
        
        self.view.addSubview(tableView)

        // Do any additional setup after loading the view.
        // Initialize central manager on load
        centralManager = CBCentralManager(delegate: self, queue: dispatch_get_main_queue())
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    
    // Check status of BLE hardware
    func centralManagerDidUpdateState(central: CBCentralManager) {
        if central.state == CBCentralManagerState.PoweredOn {
            // Scan for peripherals if BLE is turned on
            central.scanForPeripheralsWithServices(nil, options: nil)
            print("Searching for BLE Devices")
            self.statusLabel.text = "Searching for BLE Devices"
        }
        else {
            // Can have different conditions for all states if needed - print generic message for now
            print("Bluetooth switched off or not initialized")
        }
    }
    
    // Check out the discovered peripherals to find Device Tag
    func centralManager(central: CBCentralManager, didDiscoverPeripheral peripheral: CBPeripheral, advertisementData: [String : AnyObject], RSSI: NSNumber) {
        //let nameOfDeviceFound = (advertisementData as NSDictionary).objectForKey(CBAdvertisementDataLocalNameKey) as? NSString
        //print(nameOfDeviceFound)
        var nameOfDeviceFound = ""
        if let txt = peripheral.name {
            nameOfDeviceFound = txt
        }
        else {
            nameOfDeviceFound = "NOT FOUND"
        }
        if (nameOfDeviceFound != "NOT FOUND") {
            peripherals.insert(peripheral)
            tableView.reloadData()
        }
        // we need to set the name of we can check against an ID
        //print("Device Found: " + nameOfDeviceFound)
        //print("Device Name: " + deviceName)
        //print(shouldConnect)
        //print(nameOfDeviceFound)
        //if (shouldConnect) {
        //    print("Found the device!")
            // Update Status Label
        //    self.statusLabel.text = "BLE Device Tag Found"
            // Stop scanning
        //    self.centralManager.stopScan()
            // Set as the peripheral to use and establish connection
        //    self.blePeripheral = currentPeripheral
        //    self.blePeripheral.delegate = self
        //    self.centralManager.connectPeripheral(blePeripheral, options: nil)
            //print(peripheral)
        //}
        //else {
        //    self.statusLabel.text = "BLE Device Tag NOT Found"
        //}
    }
    
    // Discover services of the peripheral
    func centralManager(central: CBCentralManager, didConnectPeripheral peripheral: CBPeripheral) {
        self.statusLabel.text = "Connected"
        peripheral.discoverServices(nil)
    }
    
    // Check if the service discovered is a valid Service
    func peripheral(peripheral: CBPeripheral, didDiscoverServices error: NSError?) {
        self.statusLabel.text = "Connected"
        for service in peripheral.services! {
            let thisService = service as CBService
            if service.UUID == UARTServiceUUID {
                print("WE IN IT")
                // Discover characteristics of Adafruit
                peripheral.discoverCharacteristics(nil, forService: thisService)
            }
            // Uncomment to print list of UUIDs
            //println(thisService.UUID)
        }
    }
    
    // Get data values when they are updated
    func peripheral(peripheral: CBPeripheral, didUpdateValueForCharacteristic characteristic: CBCharacteristic, error: NSError?) {
        
        self.statusLabel.text = "Receving Data"
        
        if characteristic.UUID == RXDataUUID {
            // Convert NSData to array of signed 16 bit values
            let dataBytes = characteristic.value
            let dataLength = dataBytes!.length
            var dataArray = [UInt8](count: dataLength, repeatedValue: 0)
            dataBytes!.getBytes(&dataArray, length: dataLength * sizeof(Int16))
            if let str = String(bytes: dataArray, encoding: NSUTF8StringEncoding) {
                print(str)
            }
        }
    }
    
    func peripheral(peripheral: CBPeripheral, didDiscoverCharacteristicsForService service: CBService, error: NSError?)
    {
        //print("peripheral:\(peripheral) and service:\(service)")
        for characteristic in service.characteristics!
        {
            peripheral.setNotifyValue(true, forCharacteristic: characteristic)
        }
    }
    
    // If disconnected, start searching again
    func centralManager(central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: NSError?) {
        self.statusLabel.text = "Disconnected"
        central.scanForPeripheralsWithServices(nil, options: nil)
    }
    
    //UITableView methods
    func tableView(tableView: UITableView, cellForRowAtIndexPath indexPath: NSIndexPath) -> UITableViewCell
    {
        let cell:UITableViewCell = self.tableView.dequeueReusableCellWithIdentifier("cell")! as UITableViewCell
        
        let peripheral = peripherals[peripherals.startIndex.advancedBy(indexPath.row)]
        cell.textLabel?.text = peripheral.name! + "                       Connect"
        return cell
    }
    
    func tableView(tableView: UITableView, numberOfRowsInSection section: Int) -> Int
    {
        return peripherals.count
    }
    
    func tableView(tableView: UITableView, didSelectRowAtIndexPath indexPath: NSIndexPath) {
        //CODE TO BE RUN ON CELL TOUCH
        var peripheralName = ""
        currentPeripheral = peripherals[peripherals.startIndex.advancedBy(indexPath.row)]
        if let name = currentPeripheral!.name {
            peripheralName = name
        }
        if (peripheralName == deviceName) {
            self.statusLabel.text = "Trying to connect"
            // Stop scanning
            self.centralManager.stopScan()
            // Set as the peripheral to use and establish connection
            self.blePeripheral = currentPeripheral
            self.blePeripheral.delegate = self
            self.centralManager.connectPeripheral(blePeripheral, options: nil)
            //print(peripheral)
        }
        else {
            self.statusLabel.text = "Cannot connect to this device"
        }
    }
    
    func handleRefresh(refreshControl: UIRefreshControl) {
        self.peripherals = Set<CBPeripheral>()
        self.centralManager.scanForPeripheralsWithServices(nil, options: nil)
        self.tableView.reloadData()
        refreshControl.endRefreshing()
    }

}
