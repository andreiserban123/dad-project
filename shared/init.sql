CREATE TABLE IF NOT EXISTS pictures (
    id          BIGINT AUTO_INCREMENT PRIMARY KEY,
    filename    VARCHAR(255)    NOT NULL,
    operation   ENUM('encrypt','decrypt') NOT NULL,
    mode        VARCHAR(16)     NOT NULL DEFAULT 'CBC',
    data        LONGBLOB        NOT NULL,
    created_at  TIMESTAMP       DEFAULT CURRENT_TIMESTAMP
);
