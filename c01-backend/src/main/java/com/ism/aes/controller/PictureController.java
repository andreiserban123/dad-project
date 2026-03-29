package com.ism.aes.controller;

import com.ism.aes.config.RabbitConfig;
import com.ism.aes.messaging.NotificationService;
import org.springframework.amqp.rabbit.core.RabbitTemplate;
import org.springframework.http.ResponseEntity;
import org.springframework.messaging.simp.SimpMessagingTemplate;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.multipart.MultipartFile;

import java.util.Map;

@RestController
@RequestMapping("/api")
@CrossOrigin(origins = "*")
public class PictureController {

    private final RabbitTemplate rabbit;
    private final SimpMessagingTemplate ws;

    public PictureController(RabbitTemplate rabbit, SimpMessagingTemplate ws) {
        this.rabbit = rabbit;
        this.ws = ws;
    }

    /**
     * POST /api/process
     * Form fields:
     *   file      — BMP multipart file
     *   key       — hex string (64 chars = 32 bytes = AES-256 key)
     *   operation — "encrypt" | "decrypt"
     *   mode      — "CBC" | "ECB"
     */
    @PostMapping("/process")
    public ResponseEntity<Map<String, String>> process(
            @RequestParam("file")      MultipartFile file,
            @RequestParam("key")       String key,
            @RequestParam("operation") String operation,
            @RequestParam(value = "mode", defaultValue = "CBC") String mode
    ) throws Exception {

        byte[] bmpBytes = (file.getOriginalFilename() + "\n" +
                                  key + "\n" + operation + "\n" + mode + "\n").getBytes();
        // Pack: filename\nkey\noperation\nmode\n<raw BMP bytes>
        byte[] header   = bmpBytes;
        byte[] payload  = file.getBytes();
        byte[] message  = new byte[header.length + payload.length];
        System.arraycopy(header,  0, message, 0,             header.length);
        System.arraycopy(payload, 0, message, header.length, payload.length);

        rabbit.convertAndSend(RabbitConfig.EXCHANGE, RabbitConfig.RK_IN, message);

        return ResponseEntity.accepted().body(Map.of("status", "queued"));
    }

    /** Called internally by NotificationService when C03 signals completion */
    public void notifyFrontend(long pictureId) {
        String downloadUrl = System.getenv("C05_BASE_URL") + "/picture/" + pictureId;
        ws.convertAndSend("/topic/done", Map.of("url", downloadUrl));
    }
}
