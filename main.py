import serial
import time
import threading
import asyncio
from telegram import Bot
import os

# === CONFIGURATION ===
BOT_TOKEN = os.getenv("BOT_TOKEN")
CHAT_ID = os.getenv("CHAT_ID")
SERIAL_PORT = "COM3"
BAUD_RATE = 9600
ALERT_TEXT = "ALERT: FIRE AT PARKING!"
COOLDOWN_SECONDS = 10

# === SETUP ===
arduino = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
bot = Bot(token=BOT_TOKEN)

last_sent = 0  # Timestamp of last alert

# === TELEGRAM SEND FUNCTION ===
async def send_alert(message):
    try:
        await bot.send_message(chat_id=CHAT_ID, text=message)
        print(f"[Telegram] Sent: {message}")
    except Exception as e:
        print(f"[Error] Could not send Telegram message: {e}")

# === SERIAL READER THREAD ===
def read_serial():
    global last_sent
    while True:
        if arduino.in_waiting:
            line = arduino.readline().decode(errors="ignore").strip()
            print(f"[Arduino] {line}")  # Debug print

            if line == ALERT_TEXT:
                now = time.time()
                if now - last_sent > COOLDOWN_SECONDS:
                    # loop.call_soon_threadsafe()
                    # loop.call_soon_threadsafe(asyncio.create_task, send_alert("⚠️ Fire at parking slot!"))
                    asyncio.run(send_alert("⚠️ Fire at parking slot!"))
                    last_sent = now

        time.sleep(0.1)

# === START THREAD ===
t = threading.Thread(target=read_serial)
t.daemon = True
t.start()

print("🚀 Listening for Arduino alerts... Press Ctrl+C to stop.")
try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("\n🛑 Stopped.")
    arduino.close()