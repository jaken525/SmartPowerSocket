#!/bin/bash

if [ "$EUID" -ne 0 ]; then 
    echo "Please run as root (use sudo)"
    exit 1
fi

echo "Installing Smart Plug Server..."

cp build/smart_plug_server /usr/local/bin/

mkdir -p /etc/smart_plug
mkdir -p /var/log/smart_plug

if [ -f config/config.json ]; then
    cp config/config.json /etc/smart_plug/
else
    echo "Creating default config..."
    echo "# Smart Plug Configuration" > /etc/smart_plug/config.json
    echo "server.port=5000" >> /etc/smart_plug/config.json
    echo "server.address=0.0.0.0" >> /etc/smart_plug/config.json
    echo "gpio.pin=17" >> /etc/smart_plug/config.json
    echo "gpio.simulation=false" >> /etc/smart_plug/config.json
fi

if [ -f systemd/smart_plug.service ]; then
    cp systemd/smart_plug.service /lib/systemd/system/
    systemctl daemon-reload
    systemctl enable smart_plug.service
    echo "Systemd service installed and enabled"
fi

echo "Installation completed!"
echo "Edit config: /etc/smart_plug/config.json"
echo "Start service: sudo systemctl start smart_plug"
echo "View logs: sudo journalctl -u smart_plug -f"