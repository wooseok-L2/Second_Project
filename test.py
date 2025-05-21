from mfrc522 import SimpleMFRC522
import gpiozero

reader = SimpleMFRC522()
print("RFID 태그를 가까이 대세요...")

try:
    id, text = reader.read()
    print(f"Detected ID: {id}, Text: {text}")
except Exception as e:
    print(f"Error: {e}")