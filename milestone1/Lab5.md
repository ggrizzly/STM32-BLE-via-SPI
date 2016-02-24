# Lab5: In which this lab sucked so much, that only a select few, blessed by god himself were proven to complete this difficult task
###Team: Gleb Alexeev (galexeev), Ian Brown (irbrown)

## Overview 
This file serves as its own special snowflake. The purpose of this lab was to create a system
for transfering data over wifi, saving data sent to you, sending acknowledgements, and 
breaking the spirit of every person in 442. This documentation will be separated in three sections:
System Overview, Memory Commands, and Results.

## System Overview
Our system made use of the NRF24L01 wifi chip that we successfully added to our STM boards.
Unlike the previous, labs, this lab destroyed us not from the outside, but from within. The wifi 
through various commands would transfer data, constantly collect it coming from other people, and  
store it in an array. When you transfer the data, you would have to wait for an acknowledgement
back from the other person that they got the data. Likewise, through the ChibiOS commands of addr
and ch (which will be talked about in the next section) we are able to change our address and 
the channel on which we operate.

We have a thread operating that's constantly reading in data. It checks that the data is not empty,
and if its not, it goes through the process of locking the mutex, sending the acknowledge, and then
unlocking the mutex again. Likewise, it goes from TX mode to send it, and then back to RX mode (to
always be receiving data at all points in time).

## Commands
* **nrf ch {channel num}**: This command changes the channel that we operate on using the 
nrf24l01SetChannel(&wifiModuleDriver, value) command. Setting whatever our input of the value as
the value inside the command.

* **nrf addr {5 byte array of address}**: This command sets the RX address to whatever we choose to
be, sorta like a name for a person, but we choose it for ourselves.

* **nrf tx**: Ahhhhhhh, the wonderous, wonderful 16 hour piece I like to call: 
"Why has god abandoned us." Just kidding. It took a lot of work, but we finally were able to send
acknowledgements and receive messages. We thought that sending acknowledgements had the order of 
{message sequence}{acknowledge}{receiver address}, but turns out I think that's wrong. The wording
was weird, but both Jordan's and our group did this so we can test it.

* **nrf rx list**: Lists the information stored in the global array/buffer of messages.

* **nrf rx**: reads the lastest message, and removes it.

* **nrf rx rst**: Kind of like a test function, it resets the pointer in the queue so that if 
something goes wrong, we can test for that.

## Results
We are able to receive and trasmit data, but the connection is flakey. Weird problems kept piling
up that just didnt make sense. For example, our original addr is opium. So if we transfer from that
addr, it doesn't work. But say we do nrf addr hello, and we transfer from hello, it suddenly works.
Likewise sometimes if you receive as opium, it doesn't but change it, and it might suddenly work. I
looked at if we set our RX and TX address initially, and we do! That's the weird part. But it seems
to all work now thank god. 