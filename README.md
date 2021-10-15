# Smart-Helmet-using-NodeMCU

- NodeMCU 
- MPU6050
- GPS Module
- Adafruit IO feed connected to both Node MCU and IFTTT
- IFTTT- Google assistant (Trigger) + Adafruit IO Feed  & Webhooks + SMS

**How this works**
 - Sudden change in RMS Value of Acceleration and a constant RMS value of Orientation is considered as an accident/free fall. Incase if the helmet is dropped accidentally, the user gets to 
   stop the alert using Google assistant.( 6 sec delay to check for accidental fall)
 - If both the conditions are satisfied, values from GPS module are used to form a gmaps link which is then sent to the mentioned mobile number as an SMS through IFTTT.
 - This whole process works only when the State variable is set 'ON'in the Adafruit IO feed. The same feed is used to stop the alert for one iteration of the loop() function by setting it to 'OFF'.
