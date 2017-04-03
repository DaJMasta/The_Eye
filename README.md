# The_Eye
64 pixel discrete grayscale imager base off the Maple Mini core
By: Jonathan Zepp - @DaJMasta - http://medpants.com

April 3, 2017 first release v1.1

 ver 1.1 changes:
 Added auto gamma functionality
 Tweaked auto brightness and contrast values
 Changed the way auto brightness applies to be more useful

Included:
the_eye - Main firmware for the maple mini - does basic image improvements, auto-calibrates the highly variable CDS sensor data, and sends it all back over serial
the_eye_interface - The interface software to be run on the connected computer, displays the image, gives basic control over settings and built in lights, and reports general status and adjustment information

Dependancies:
Built in Arduino iDE 1.6.5
Uses Arduino_STM32 for the maple port to the Arduino IDE

Interface built in Processing 3.0.1

For more information, pictures, and video of some of the features, the project's homepage is here:
http://medpants.com/the-eye-robot
