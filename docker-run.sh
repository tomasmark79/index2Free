#!/bin/bash

export USER_ID=$(id -u)
export GROUP_ID=$(id -g)
export USERNAME=$(whoami)

echo "Starting container with:"
echo "  USER_ID: $USER_ID"
echo "  GROUP_ID: $GROUP_ID"
echo "  USERNAME: $USERNAME"

# Nejdříve zastavíme a odstraníme případný běžící kontejner
docker-compose -f docker-compose.yml down

# Spustíme s logováním pro debug
docker-compose -f docker-compose.yml up --build -d

# Počkáme chvilku, než se kontejner spustí
sleep 2

# Zkusíme se připojit a automaticky přepnout na uživatele tomas
echo "Attempting to connect to container..."
if docker-compose -f docker-compose.yml exec dockerhell su - ${USERNAME}; then
    echo "Connected successfully!"
else
    echo "Failed to connect. Checking logs..."
    docker-compose -f docker-compose.yml logs dockerhell
    echo "Container status:"
    docker-compose -f docker-compose.yml ps
fi
