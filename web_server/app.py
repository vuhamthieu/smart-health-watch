from flask import Flask, render_template, request, jsonify
import json
import os
import time
import threading
from paho.mqtt import client as mqtt

app = Flask(__name__)
DATA_FILE = "data.json"

def read_data():
    if not os.path.exists(DATA_FILE):
        save_data({"heart_rate": 0, "spo2": 0, "temperature": 0, "latitude": 0, "longitude": 0})
    with open(DATA_FILE, "r") as f:
        return json.load(f)

def save_data(data):
    with open(DATA_FILE, "w") as f:
        json.dump(data, f, indent=4)

def start_mqtt():
    def on_message(c, u, msg):
        try:
            data = json.loads(msg.payload.decode())
            data["_last_ts"] = time.time()
            cur = read_data(); cur.update(data); save_data(cur)
        except Exception as e:
            print(f"MQTT on_message error: {e}")

    c = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1,
                    client_id="flask-subscriber",
                    protocol=mqtt.MQTTv311)
    c.on_message = on_message
    c.connect("192.168.1.60", 1883, 60)
    c.subscribe("health_monitor/device01/#", qos=1)
    c.loop_start()

@app.route("/")
def index():
    st = "Disconnected"
    try:
        d = read_data()
        if d.get("_last_ts") and (time.time() - d["_last_ts"]) < 5:
            st = "Connected"
    except Exception:
        pass
    return render_template("index.html", connection_status=st)

@app.route("/update", methods=["POST"])
def update_data():
    if not request.is_json:
        return jsonify({"error": "Invalid JSON"}), 400
    data = request.get_json()
    if not data:
        return jsonify({"error": "No data provided"}), 400
    current_data = read_data()
    current_data.update(data)
    save_data(current_data)
    return jsonify({"status": "success"}), 200

@app.route("/get_data", methods=["GET"])
def get_data():
    try:
        return jsonify(read_data())
    except Exception as e:
        print(f"Error in get_data: {e}")
        return jsonify({"error": "Server error"}), 500

if __name__ == "__main__":
    threading.Thread(target=start_mqtt, daemon=True).start()
    app.run(host="0.0.0.0", port=5000, debug=False)
