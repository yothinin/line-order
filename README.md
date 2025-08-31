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
env

.
├── listbox.c        # main source code
├── .env             # environment variables
├── README.md        # project documentation


## 📂 Project Structure
.
├── listbox.c # main source code
├── .env # environment variables
├── README.md # project documentation


## 📄 License
MIT License
