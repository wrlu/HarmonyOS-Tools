from flask import Flask, request
import os
import platform

app = Flask(__name__)

@app.route('/upload_from_ohos', methods=['POST'])
def upload_file():
    if request.method == 'POST':
        file = request.files['file']
        if file:
            filename = file.filename
            file.save(os.path.join('uploads', filename))
            return '0'
    return '1'

if __name__ == '__main__':
    if 'Windows' in platform.system():
        os.system('ipconfig')
    else:
        os.system('ip addr')
    if not os.path.exists('uploads'):
        os.makedirs('uploads')
    app.run(host="192.168.8.42", port=5000)
