from flask import Flask, jsonify, request, send_from_directory
import os

app = Flask(__name__)

dados_atuais = {
    'x': 0,
    'y': 0,
    'btn_a': 0,
    'btn_b': 0,
    'gas': 0,
    'dir': 'norte'
}

@app.route('/update', methods=['POST'])
def update_data():
    global dados_atuais
    try:
        payload = request.get_json(force=True)
        required_fields = ['x', 'y', 'btn_a', 'btn_b', 'gas', 'dir']
        if all(field in payload for field in required_fields):
            dados_atuais = payload
            return jsonify({'status': 'ok'}), 200
        return jsonify({'erro': 'Formato inválido'}), 400
    except Exception as e:
        return jsonify({'erro': str(e)}), 500

@app.route('/data', methods=['GET'])
def get_data():
    return jsonify(dados_atuais)

# Página principal
@app.route('/')
def index():
    return send_from_directory('front-end', 'index.html')

# Arquivos estáticos (CSS, JS, imagens etc.)
@app.route('/<path:path>')
def serve_static(path):
    return send_from_directory('front-end', path)

# Rota de debug para verificar conteúdo da pasta
@app.route('/debug-files')
def debug_files():
    caminho = os.path.join(os.getcwd(), 'front-end')
    try:
        arquivos = os.listdir(caminho)
        return jsonify({'arquivos': arquivos})
    except Exception as e:
        return jsonify({'erro': str(e)})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8050, debug=True)