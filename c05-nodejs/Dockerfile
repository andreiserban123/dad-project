FROM critoma/amd64_u24_noble_ism_security

ENV NODE_HOME=/opt/software/nodejs/node-v22.21.0-linux-x64
ENV PATH=$NODE_HOME/bin:$PATH

WORKDIR /app

# Install dependencies first (layer cache)
COPY package.json .
RUN npm install --omit=dev

# Copy source_
COPY src ./src

EXPOSE 3000
CMD ["node", "src/index.js"]