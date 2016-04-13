//
//  TrailData.swift
//  TrailCounterApp
//
//  Created by Jordan Ventura Gaeta on 4/12/16.
//  Copyright © 2016 p442. All rights reserved.
//
import UIKit

class TrailData {
    
    var dataDict = Dictionary<String, Array<Int>>()
    
    func getFileURL(fileName: String) -> String {
        let manager = NSFileManager.defaultManager()
        do {
            let dirURL = try manager.URLForDirectory(.DocumentDirectory, inDomain: .UserDomainMask, appropriateForURL: nil, create: false)
            return dirURL.URLByAppendingPathComponent(fileName).path!
        }
        catch {
            print("woops")
        }
        return "NULL"
    }
    
    func retrieveData() {
        let fm = NSFileManager()
        let filePath = getFileURL("data")
        if fm.fileExistsAtPath(filePath) {
            self.dataDict = NSKeyedUnarchiver.unarchiveObjectWithFile(filePath) as! Dictionary<String, Array<Int>>
        }
    }
    
    func saveData(data: Array<Int>) {
        self.dataDict[String(self.dataDict.count)] = data
        let filePath = getFileURL("data")
        NSKeyedArchiver.archiveRootObject(self.dataDict, toFile: filePath)
    }
    
    func removeData() {
        self.dataDict = [:]
        let fm = NSFileManager()
        let filePath = getFileURL("data")
        if fm.fileExistsAtPath(filePath) {
            do {
                try fm.removeItemAtPath(filePath)
            }
            catch {
                print("woops.")
            }
        }
    }
    
    
}
