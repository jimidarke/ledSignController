#!/bin/bash
# Alert Manager MQTT Client Certificate Installation

INSTALL_DIR="/opt/alert-manager/certs"
CONFIG_DIR="/opt/alert-manager/config"

echo "Installing Alert Manager MQTT client certificates..."

# Create directories
sudo mkdir -p "$INSTALL_DIR" "$CONFIG_DIR"

# Install certificates
sudo cp ca.crt "$INSTALL_DIR/"
sudo cp client.crt "$INSTALL_DIR/"
sudo cp client.key "$INSTALL_DIR/"
sudo cp mqtt-config.json "$CONFIG_DIR/"

# Set permissions
sudo chmod 644 "$INSTALL_DIR/ca.crt" "$INSTALL_DIR/client.crt"
sudo chmod 600 "$INSTALL_DIR/client.key"
sudo chmod 644 "$CONFIG_DIR/mqtt-config.json"

# Set ownership (adjust user as needed)
sudo chown -R alert:alert "$INSTALL_DIR" "$CONFIG_DIR" 2>/dev/null || true

echo "Installation complete!"
echo "CA Certificate: $INSTALL_DIR/ca.crt"
echo "Client Certificate: $INSTALL_DIR/client.crt"
echo "Client Key: $INSTALL_DIR/client.key"
echo "MQTT Config: $CONFIG_DIR/mqtt-config.json"
