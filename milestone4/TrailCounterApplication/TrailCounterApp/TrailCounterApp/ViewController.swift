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
    
    var peripherals: Set<CBPeripheral> = Set<CBPeripheral>()
    
    var titleLabel : UILabel!
    var statusLabel : UILabel!
    var tableView: UITableView  =   UITableView()
    
    // BLE
    var centralManager : CBCentralManager!
    var blePeripheral : CBPeripheral!
    
    // IR Temp UUIDs
    let RXDataUUID = CBUUID(string: "6E400003-B5A3-F393-E0A9-E50E24DCCA9E")
    let TXDataUUID   = CBUUID(string: "6E400002-B5A3-F393-E0A9-E50E24DCCA9E")
    let UARTServiceUUID = CBUUID(string: "6E400001-B5A3-F393-E0A9-E50E24DCCA9E")

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
        
        self.view.addSubview(tableView)

        // Do any additional setup after loading the view.
        // Initialize central manager on load
        centralManager = CBCentralManager(delegate: self, queue: nil)
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
            self.statusLabel.text = "Searching for BLE Devices"
        }
        else {
            // Can have different conditions for all states if needed - print generic message for now
            print("Bluetooth switched off or not initialized")
        }
    }
    
    // Check out the discovered peripherals to find Device Tag
    func centralManager(central: CBCentralManager, didDiscoverPeripheral peripheral: CBPeripheral, advertisementData: [String : AnyObject], RSSI: NSNumber) {
        peripherals.insert(peripheral)
        tableView.reloadData()
        let deviceName = "BLE Device Name"
        //let nameOfDeviceFound = (advertisementData as NSDictionary).objectForKey(CBAdvertisementDataLocalNameKey) as? NSString
        //print(nameOfDeviceFound)
        var nameOfDeviceFound = ""
        if let txt = peripheral.name {
            nameOfDeviceFound = txt
        }
        else {
            nameOfDeviceFound = "NOT FOUND"
        }
        // we need to set the name of we can check against an ID
        print("Device Found: " + nameOfDeviceFound)
        print("Device Name: " + deviceName)
        if (nameOfDeviceFound == deviceName) {
            print("Found the device!")
            // Update Status Label
            self.statusLabel.text = "BLE Device Tag Found"
            
            // Stop scanning
            self.centralManager.stopScan()
            // Set as the peripheral to use and establish connection
            self.blePeripheral = peripheral
            self.blePeripheral.delegate = self
            self.centralManager.connectPeripheral(peripheral, options: nil)
        }
        else {
            self.statusLabel.text = "BLE Device Tag NOT Found"
        }
    }
    
    // Discover services of the peripheral
    func centralManager(central: CBCentralManager, didConnectPeripheral peripheral: CBPeripheral) {
        self.statusLabel.text = "Discovering peripheral services"
        peripheral.discoverServices(nil)
    }
    
    // Check if the service discovered is a valid Service
    func peripheral(peripheral: CBPeripheral, didDiscoverServices error: NSError?) {
        self.statusLabel.text = "Looking at peripheral services"
        for service in peripheral.services! {
            let thisService = service as CBService
            if service.UUID == UARTServiceUUID {
                // Discover characteristics of IR Temperature Service
                peripheral.discoverCharacteristics(nil, forService: thisService)
            }
            // Uncomment to print list of UUIDs
            //println(thisService.UUID)
        }
    }
    
    // Get data values when they are updated
    func peripheral(peripheral: CBPeripheral, didUpdateValueForCharacteristic characteristic: CBCharacteristic, error: NSError?) {
        
        self.statusLabel.text = "Connected"
        
        if characteristic.UUID == TXDataUUID {
            // Convert NSData to array of signed 16 bit values
            let dataBytes = characteristic.value
            let dataLength = dataBytes!.length
            var dataArray = [Int16](count: dataLength, repeatedValue: 0)
            dataBytes!.getBytes(&dataArray, length: dataLength * sizeof(Int16))
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
        cell.textLabel?.text = peripheral.name
        
        return cell
    }
    
    func tableView(tableView: UITableView, numberOfRowsInSection section: Int) -> Int
    {
        return peripherals.count
    }

}
