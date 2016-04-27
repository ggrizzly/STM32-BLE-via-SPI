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
    
    @IBOutlet var uploadButton : UIButton!
    @IBOutlet var titleLabel : UILabel!
    @IBOutlet var statusLabel : UILabel!
    var tableLabel : UILabel!
    @IBOutlet var tableView : UITableView!
    var tData = TrailData()
    var isFirst = 1
    var size = 0
    var counter = 0
    var date : NSDate?
    var dateNotWritten = 1
    
    lazy var refreshControl: UIRefreshControl = {
        let refreshControl = UIRefreshControl()
        refreshControl.addTarget(self, action: #selector(ViewController.handleRefresh(_:)), forControlEvents: UIControlEvents.ValueChanged)
        
        return refreshControl
    }()
    
    var peripherals: Set<CBPeripheral> = Set<CBPeripheral>()
    var txCharacteristic:CBCharacteristic?
    let alert = UIAlertController(title: "Downloading Data: Do NOT disconnect", message: "0%", preferredStyle: UIAlertControllerStyle.Alert)
    let alert2 = UIAlertController(title: "Sending Data to server.", message: "", preferredStyle: UIAlertControllerStyle.Alert)
    //var shouldConnect = false
    
    // BLE
    var centralManager : CBCentralManager!
    var blePeripheral : CBPeripheral!
    
    // UUIDS for services offered by BLE device
    let RXDataUUID = CBUUID(string: "6E400003-B5A3-F393-E0A9-E50E24DCCA9E")
    let TXDataUUID   = CBUUID(string: "6E400002-B5A3-F393-E0A9-E50E24DCCA9E")
    let UARTServiceUUID = CBUUID(string: "6E400001-B5A3-F393-E0A9-E50E24DCCA9E")
    let deviceName = "Adafruit Bluefruit LE"
    var data = Array<Int>()

    override func viewDidLoad() {
        super.viewDidLoad()
        
        titleLabel.text = "AdaFruit Data Retriever"
        titleLabel.font = UIFont(name: "HelveticaNeue-Bold", size: 20)
        titleLabel.textAlignment = NSTextAlignment.Center
        
        statusLabel.textAlignment = NSTextAlignment.Center
        statusLabel.text = "Loading..."
        statusLabel.font = UIFont(name: "HelveticaNeue-Light", size: 12)
        
        tableView.delegate = self
        tableView.dataSource = self
        
        tableView.registerClass(UITableViewCell.self, forCellReuseIdentifier: "cell")
        self.tableView.addSubview(self.refreshControl)

        centralManager = CBCentralManager(delegate: self, queue: dispatch_get_main_queue())
        alert.addAction(UIAlertAction(title: "Disconnect", style: .Default, handler: { action in
            switch action.style{
            case .Default:
                self.disconnectHandler()
            case .Cancel:
                self.disconnectHandler()
            case .Destructive:
                self.disconnectHandler()
            }
        }))
        
        self.date = NSDate()
    }
    
    func sendDataToServer() throws {
        self.presentViewController(alert2, animated: true, completion: nil)
        self.tData.retrieveData()
        let json = ["title":"Trail Counter Data P442" , "Data": tData.dataDict]
        let jsonData = try NSJSONSerialization.dataWithJSONObject(json, options: .PrettyPrinted)
        
        // create post request
        let url = NSURL(string: "http://burrow.soic.indiana.edu:50000/data.php")!
        let request = NSMutableURLRequest(URL: url)
        request.HTTPMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        
        // insert json data to the request
        request.HTTPBody = jsonData
        
        let task = NSURLSession.sharedSession().dataTaskWithRequest(request, completionHandler: { (data: NSData?, response:NSURLResponse?,
            error: NSError?) -> Void in
              self.alert2.dismissViewControllerAnimated(true, completion: nil)
              print(error?.code)
            })
        
        task.resume()
    }
    
    func disconnectHandler() {
        self.counter = 0
        self.isFirst = 1
        self.dateNotWritten = 1
        self.statusLabel.text = "Disconnected"
        data = Array<Int>()
        self.alert.message = "0%"
        self.tableView.reloadData()
        self.centralManager.cancelPeripheralConnection(self.blePeripheral)
        self.peripherals = Set<CBPeripheral>()
        self.tableView.reloadData()
    }
    
    @IBAction func buttonAction(sender: UIButton) {
        // send the stored data to web server and then delete it
        print("Going to try")
        do {
            try self.sendDataToServer()
            self.tData.removeData()
            print("Sent")
        } catch { print("DID NOT SEND") }
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
            // print this since the device is not powered on
            print("Bluetooth switched off or not initialized")
        }
    }
    
    // Check out the discovered peripherals to find Device Tag
    func centralManager(central: CBCentralManager, didDiscoverPeripheral peripheral: CBPeripheral, advertisementData: [String : AnyObject], RSSI: NSNumber) {
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
            dataBytes!.getBytes(&dataArray, length: dataLength * sizeof(UInt8))
            if isFirst == 1 {
                if let str = String(bytes: dataArray, encoding: NSUTF8StringEncoding) {
                    if str.characters.count < 6 {
                        self.size = Int(str)!
                    }
                    else {
                        self.size = 0
                        self.disconnectHandler()
                    }
                }
                isFirst = 0
            }
            else {
                if self.counter <= self.size {
                    for d in dataArray {
                        // this is just a temporary encoding since we are testing
                        // by sending ints
                        if let temp = String(bytes: [d], encoding: NSUTF8StringEncoding) {
                           data.append(Int(temp)!)
                        }
                        else {
                            data.append(-1)
                        }
                        self.counter = self.counter + 1
                        self.alert.message = String(Float(self.counter) / Float(self.size) * 100.0) + "%"
                    }
                    if self.counter >= self.size {
                        self.tData.saveData(data)
                        self.disconnectHandler()
                        self.alert.dismissViewControllerAnimated(true, completion: nil)
                    }
                }
            }
        }
    }
    
    func peripheral(peripheral: CBPeripheral, didDiscoverCharacteristicsForService service: CBService, error: NSError?) {
        
        for characteristic in service.characteristics! {
            if (characteristic.UUID == TXDataUUID) {
                txCharacteristic = characteristic
            }
            peripheral.setNotifyValue(true, forCharacteristic: characteristic)
        }
        
        if self.dateNotWritten == 1 {
            self.date = NSDate()
            let timeData = String(self.date)
            print("trying to write date")
            writeString(timeData)
            print("wrote data")
            self.dateNotWritten = 0
        }
    }
    
    func writeString(string: NSString) {
        let data = NSData(bytes: string.UTF8String, length: string.length)
        writeRawData(data)
    }
    
    // If disconnected, start searching again
    func centralManager(central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: NSError?) {
        self.statusLabel.text = "Disconnected"
        self.counter = 0
        self.isFirst = 1
        self.dateNotWritten = 1
        data = Array<Int>()
        self.alert.message = "0%"
        self.peripherals = Set<CBPeripheral>()
        self.alert.dismissViewControllerAnimated(true, completion: nil)
        central.scanForPeripheralsWithServices(nil, options: nil)
    }
    
    //UITableView methods
    func tableView(tableView: UITableView, cellForRowAtIndexPath indexPath: NSIndexPath) -> UITableViewCell {
        let cell:UITableViewCell = self.tableView.dequeueReusableCellWithIdentifier("cell")! as UITableViewCell
        
        let peripheral = peripherals[peripherals.startIndex.advancedBy(indexPath.row)]
        cell.textLabel?.text = peripheral.name!
        return cell
    }
    
    func tableView(tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return peripherals.count
    }
    
    func tableView(tableView: UITableView, didSelectRowAtIndexPath indexPath: NSIndexPath) {
        //CODE TO BE RUN ON CELL TOUCH
        var peripheralName = ""
        self.blePeripheral = peripherals[peripherals.startIndex.advancedBy(indexPath.row)]
        if let name = self.blePeripheral!.name {
            peripheralName = name
        }
        if (peripheralName == deviceName) {
            self.statusLabel.text = "Trying to connect"
            // Stop scanning
            self.centralManager.stopScan()
            // Set as the peripheral to use and establish connection
            self.blePeripheral.delegate = self
            self.centralManager.connectPeripheral(blePeripheral, options: nil)
            self.presentViewController(alert, animated: true, completion: nil)
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
    
    func peripheral(peripheral: CBPeripheral, didWriteValueForCharacteristic characteristic: CBCharacteristic, error: NSError?) {
        print(error)
    }
    
    // i did not write this, just modified for my usage
    //  Created by Collin Cunningham on 10/29/14.
    //  Copyright (c) 2014 Adafruit Industries. All rights reserved.
    func writeRawData(data:NSData) {
        
        var writeType:CBCharacteristicWriteType
            
        writeType = CBCharacteristicWriteType.WithResponse
        
        
        //TODO: Test packetization
        
        //send data in lengths of <= 20 bytes
        let dataLength = data.length
        let limit = 20
        
        //Below limit, send as-is
        if dataLength <= limit {
            print("writing")
            print(dataLength)
            print(txCharacteristic)
            self.blePeripheral.writeValue(data, forCharacteristic: txCharacteristic!, type: writeType)
        }
            
            //Above limit, send in lengths <= 20 bytes
        else {
            
            var len = limit
            var loc = 0
            var idx = 0 //for debug
            
            while loc < dataLength {
                
                let rmdr = dataLength - loc
                if rmdr <= len {
                    len = rmdr
                }
                
                let range = NSMakeRange(loc, len)
                var newBytes = [UInt8](count: len, repeatedValue: 0)
                data.getBytes(&newBytes, range: range)
                let newData = NSData(bytes: newBytes, length: len)
                self.blePeripheral.writeValue(newData, forCharacteristic: self.txCharacteristic!, type: writeType)
                loc += len
                idx += 1
            }
        }
        
    }

}
