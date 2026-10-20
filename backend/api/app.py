from flask import Flask, request, jsonify
import subprocess
import os
import re

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
C_DIR = os.path.join(ROOT, "backend", "c")
DATA_DIR = os.path.join(ROOT, "data")
os.makedirs(DATA_DIR, exist_ok=True)
CSV_PATH = os.path.join(DATA_DIR, "students.csv")
EXE = os.path.join(C_DIR, "merit_engine.exe" if os.name == "nt" else "merit_engine")

if not os.path.exists(CSV_PATH):
    with open(CSV_PATH, "w", encoding="utf-8", newline="") as f:
        f.write("name,roll,branch,marks\n")
        f.write("Aarav Sharma,221CS045,CSE,94.8\n")
        f.write("Diya Patel,221CS012,CSE,92.4\n")
        f.write("Ishaan Reddy,221EC031,ECE,88.9\n")
        f.write("Meera Rao,221IT008,IT,85.6\n")

app = Flask(__name__, static_folder=os.path.join(ROOT, "frontend"), static_url_path="")

def run_engine(*args):
    try:
        result = subprocess.run([EXE, *map(str, args)], cwd=os.path.join(ROOT, "backend", "api"),
                                capture_output=True, text=True, timeout=10)
        payload = result.stdout.strip()
        if not payload:
            return {"ok": False, "error": result.stderr.strip() or "C engine returned no response."}, 500
        data = __import__("json").loads(payload)
        return data, (200 if data.get("ok") else 400)
    except FileNotFoundError:
        return {"ok": False, "error": "C engine not compiled. Follow README instructions to build merit_engine."}, 500
    except subprocess.TimeoutExpired:
        return {"ok": False, "error": "C engine timed out."}, 504
    except Exception as exc:
        return {"ok": False, "error": f"Backend error: {exc}"}, 500

@app.get("/")
def home():
    return app.send_static_file("index.html")

@app.get("/api/students")
def students():
    data, status = run_engine("list")
    return jsonify(data), status

@app.post("/api/students")
def add_student():
    body = request.get_json(silent=True) or {}
    name = str(body.get("name", "")).strip()
    roll = str(body.get("roll", "")).strip()
    branch = str(body.get("branch", "")).strip().upper()
    marks = body.get("marks")
    if not name or not roll or not branch or len(name) > 99 or len(roll) > 39 or len(branch) > 19:
        return jsonify({"ok": False, "error": "Enter a valid name, roll number, and branch."}), 400
    if any(ch in name + roll + branch for ch in ",\r\n"):
        return jsonify({"ok": False, "error": "Text fields cannot contain commas or line breaks in this CSV version."}), 400
    try:
        value = float(marks)
        if value < 0 or value > 100:
            raise ValueError()
    except (TypeError, ValueError):
        return jsonify({"ok": False, "error": "Marks must be a number from 0 to 100."}), 400
    data, status = run_engine("add", name, roll, branch, f"{value:.2f}")
    return jsonify(data), status

@app.get("/api/merit")
def merit():
    algorithm = request.args.get("algorithm", "merge").lower()
    if algorithm not in ("merge", "quick"):
        return jsonify({"ok": False, "error": "algorithm must be merge or quick"}), 400
    data, status = run_engine("sort", algorithm)
    return jsonify(data), status

@app.get("/api/analytics")
def analytics():
    data, status = run_engine("analytics")
    return jsonify(data), status

@app.get("/api/benchmark")
def benchmark():
    data, status = run_engine("benchmark")
    return jsonify(data), status

if __name__ == "__main__":
    app.run(host="127.0.0.1", port=5000, debug=True)
