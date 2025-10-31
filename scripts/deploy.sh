#!/bin/bash

set -e

echo "🚀 Deploying GoQuant Matching Engine"

CONFIG_PATH=${1:-"config/production.json"}
DOCKER_COMPOSE_FILE="scripts/docker/docker-compose.yml"

if [ ! -f "$CONFIG_PATH" ]; then
    echo "❌ Config file not found: $CONFIG_PATH"
    exit 1
fi

if [ ! -f "$DOCKER_COMPOSE_FILE" ]; then
    echo "❌ Docker compose file not found: $DOCKER_COMPOSE_FILE"
    exit 1
fi

echo "📦 Building Docker images..."
docker-compose -f $DOCKER_COMPOSE_FILE build

echo "🛑 Stopping existing services..."
docker-compose -f $DOCKER_COMPOSE_FILE down || true

echo "✨ Starting services..."
docker-compose -f $DOCKER_COMPOSE_FILE up -d

echo "⏳ Waiting for services to start..."
sleep 10

echo "🔍 Checking service health..."
curl -f http://localhost:9001/health || {
    echo "❌ Service health check failed"
    docker-compose -f $DOCKER_COMPOSE_FILE logs matching-engine
    exit 1
}

echo "✅ Deployment completed successfully!"
echo ""
echo "📊 Services:"
echo "   Matching Engine: http://localhost:9001/health"
echo "   Prometheus:      http://localhost:9090"
echo "   Grafana:         http://localhost:3000 (admin/admin)"
echo ""
echo "📈 View metrics and monitoring at http://localhost:3000"