from flask import Flask, request, jsonify

app = Flask(__name__)

@app.route('/notify_gas_level', methods=['POST'])
def notify_gas_level():
    data = request.get_json()
    if not data or 'gas_level' not in data:
        return jsonify({"error": "No gas level provided"}), 400
    gas_level = data['gas_level']
    # Realize alguma ação com o nível de gás aqui
    print(f"Gas level received: {gas_level}")
    return jsonify({"status": "success", "gas_level": gas_level}), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
