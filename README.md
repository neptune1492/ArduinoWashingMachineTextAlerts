# ArduinoWashingMachineTextAlerts

For this project, I developed an Arduino script to send the user a text message when the washing machine had completed its cycle.  The Arduino integrated an accelerometer sensor and a WiFi shield.  I collected accelerometer data from the washing machine and developed a sliding-window classifier to determine whether data points indicated the washing machine cycle was complete. The Carriots API was used to send the text message alert.
