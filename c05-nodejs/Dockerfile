FROM critoma/amd64_u24_noble_ism_security

# Install Node.js 20 LTS
RUN apt-get update && apt-get install -y curl && \
    curl -fsSL https://deb.nodesource.com/setup_20.x | bash - && \
    apt-get install -y nodejs && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Install dependencies first (layer cache)
COPY package.json .
RUN npm install --omit=dev

# Copy source
COPY src ./src

EXPOSE 3000
CMD ["node", "src/index.js"]
