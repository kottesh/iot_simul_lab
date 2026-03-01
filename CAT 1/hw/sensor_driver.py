import RPi.GPIO as GPIO
import time
import requests
from datetime import datetime

GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

TRIG = 23
ECHO = 24

GPIO.setup(TRIG, GPIO.OUT)
GPIO.setup(ECHO, GPIO.IN)

THINGSPEAK_API_KEY = "V2DKZ0LSWTP42FJF"
THINGSPEAK_URL = "https://api.thingspeak.com/update"


def get_distance():
    GPIO.output(TRIG, False)
    time.sleep(0.5)

    GPIO.output(TRIG, True)
    time.sleep(0.00001)
    GPIO.output(TRIG, False)

    timeout = time.time() + 1
    while GPIO.input(ECHO) == 0:
        pulse_start = time.time()
        if pulse_start > timeout:
            return None

    timeout = time.time() + 1
    while GPIO.input(ECHO) == 1:
        pulse_end = time.time()
        if pulse_end > timeout:
            return None

    pulse_duration = pulse_end - pulse_start
    distance = pulse_duration * 17150
    distance = round(distance, 2)

    return distance


def upload_to_thingspeak(distance):
    payload = {"api_key": THINGSPEAK_API_KEY, "field1": distance}
    try:
        response = requests.get(THINGSPEAK_URL, params=payload)
        if response.status_code == 200:
            return True
    except:
        pass
    return False


try:
    counter = 0
    while True:
        dist = get_distance()
        if dist is not None:
            print(
                f"Distance: {dist} cm | Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}"
            )
            if counter % 15 == 0:
                upload_to_thingspeak(dist)
                print("  -> Uploaded to ThingSpeak")
        else:
            print(
                f"Timeout - No reading | Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}"
            )

        counter += 1
        time.sleep(1)

except KeyboardInterrupt:
    print("\nMeasurement stopped by user")
    GPIO.cleanup()
