# 🔐 IoT Credential & Data Display System with Firebase Integration

This is a simple and intuitive web-based Firebase IoT control panel built using HTML, CSS, and Firebase Realtime Database + Authentication.

It allows users to:
- ✅ Log in using email/password authentication
- ✅ Sign up with validation
- ✅ Send and update text messages stored in Firebase
- ✅ Display message updates on an OLED device via ESP32
- ✅ Manage WiFi credentials (EEPROM configuration page)

--

## 🚀 How to Run the Web App
### 🛠️ Steps to Run:
1. Clone or download this repository.
2. Open the folder in **VS Code**.
3. Install the Live Server extension (by Ritwick Dey).
4. Open `index.html`, right-click, and choose: Open with Live Server
5. The app should open at: http://127.0.0.1:5500/
6. Replace the your own Firebase config in `index.html`:
```javascript
const firebaseConfig = {
  apiKey: "YOUR_API_KEY",
  databaseURL: "YOUR_DATABASE_URL",
  projectId: "YOUR_PROJECT_ID"
};

--

## 📸 UI Preview

| Section             | Preview                                                                 |
|---------------------|-------------------------------------------------------------------------|
| 🔐 **Login Section**     | ![Login]()                                          |
| 👤 **Sign Up Section**   | ![Sign Up](./preview/signup.png)                                      |
| 📝 **App Section**       | ![App](./preview/app.png)                                              |
| 📶 **WiFi Config Page**  | ![image](https://github.com/user-attachments/assets/4d5d8e31-f9c1-4312-b958-863b074d724b
)                              |
