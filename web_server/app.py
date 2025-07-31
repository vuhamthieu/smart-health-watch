from flask import Flask, render_template, request, jsonify
import json
import os

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

@app.route("/")
def index():
    return render_template("index.html")

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
    app.run(host="0.0.0.0", port=5000, debug=True)