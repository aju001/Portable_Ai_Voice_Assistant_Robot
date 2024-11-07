# Portable AI Voice Assistant Robot with Gemini 🤖🎤
A low-cost AI-powered voice assistant based on the ESP32 microcontroller, featuring Gemini AI for smart conversations and Deepgram's Speech-to-Text API for seamless voice interactions. 🧠🔊 The combination of Gemini AI and Deepgram creates an interactive and responsive voice assistant, while the ESP32 keeps it affordable and efficient. 🌟



# 🔥 Features ✨
- 🎤 Offline Wake Word Detection: Using the INMP441 I2S microphone (currently in development, may use push button for now)
- 🕹️ Push-to-Ask: Press a button to trigger voice commands
- 🗣️ Speech-to-Text: Record and send queries to Deepgram for transcription
- ⚡ Real-Time AI Responses: Powered by Gemini AI for intelligent, real-time answers
- 💬 Natural Language Processing: Understands user queries and responds in natural language
- 🗣️ Text-to-Speech Output: Converts AI responses into high-quality speech via Gemini
- 🎧 Audio Playback: Voice responses are played on the ESP32 using a MAX98357A I2S amplifier & speaker
- 💾 External SD Card: Stores longer queries and additional data
- 🔁 Repeat Button: Replay the last response with one button press
- 🛠️ Easy Assembly: Utilizes common components, including Dupont connectors for fast prototyping and testing

# 🚧 Active Development Focus 🛠️
- 🔋  Power Supply Design
- 🎙️ Wake Word Detection Accuracy
- ⚡ Reducing Latency for faster responses

# 🖥️ Hardware Components 🧰
- ESP32 DevKit V1
- MAX98357A I2S 3W Class D Amplifier
- INMP441 I2S Microphone Module
- Micro SD Card Module for storing audio files
- SG90 Servo Motor
- RGB LED for status indication
- 134N3P 5V Step-Up Power Module
- TP4056 Lithium Battery Charging Module
- 3.7V Li-ion Battery
- Push Button for triggering queries
- 10K Resistors
- Slide Switch for power control

# 📊 Circuit Diagram 🖱️
Here’s a circuit diagram to help with the wiring:

![Circuit Diagram](assests/circuitdiagram.jpeg)

# 🏁 Getting Started 🚀
- 🔌 Connect the Hardware: Follow the wiring diagram to connect all components
- 💻 Install Required Software: Install the ESP32 Arduino Core and libraries
- 🌐 Configure APIs: Set up WiFi, Gemini API, and Deepgram API credentials
- ⚡ Flash the Firmware to your ESP32
- 🎤 Power On and test by holding the button and speaking a question!


# 🤩 Final Model 🔥
Here’s how your AI assistant looks when completed:

<img src="assests/bot.jpeg" alt="bot" width="400"/>

# 🚀 Exciting Potential 🌟
This voice assistant is a work-in-progress, and we’re excited about the possibilities! Future updates will bring better performance, additional features, and an even more polished user experience. Keep an eye out for updates as we continue to evolve this project! 🔧💡
