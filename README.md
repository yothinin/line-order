อ๋อ เข้าใจแล้วครับ 🙏
คุณอยากได้เป็น บล็อกเดียว (โค้ดบล็อกเดียวทั้งไฟล์) จะได้ copy ทีเดียวแล้ววางเป็น README.md ได้เลย

งั้นผมรวมให้เลยครับ ⬇️

# 📦 Line Order System (GTK)

โปรแกรมจัดการออเดอร์ด้วย **GTK+3**, **JSON-GLib**, และ **cURL**  
รองรับการตั้งค่า `.env` เพื่อความยืดหยุ่นในการใช้งาน

## ⚙️ Installation
ติดตั้ง dependencies (Debian/Ubuntu):


sudo apt update
sudo apt install -y gcc libgtk-3-dev libjson-glib-dev libcurl4-openssl-dev pkg-config


## 🛠️ Build
คอมไพล์ด้วย:


gcc listbox.c -o listbox pkg-config --cflags --libs gtk+-3.0 json-glib-1.0 -lcurl -Wall


## 🚀 Run
รันโปรแกรม:


./listbox


## 📝 Config (.env)
ไฟล์ `.env` ต้องอยู่ในโฟลเดอร์เดียวกับ binary:  

```env
MACHINE_NAME=my-machine
TOKEN=my-secret-token
API_BASE_URL=https://api.example.com
```

## 📂 Project Structure
```
.
├── listbox.c # main source code
├── .env # environment variables
├── README.md # project documentation
```

## 📄 License
MIT License

Copyright (c) 2025 Yothin Inbanleng

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

