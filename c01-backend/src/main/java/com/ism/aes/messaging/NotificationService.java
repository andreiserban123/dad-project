package com.ism.aes.messaging;

import com.ism.aes.config.RabbitConfig;
import com.ism.aes.controller.PictureController;
import org.springframework.amqp.rabbit.annotation.RabbitListener;
import org.springframework.stereotype.Service;

@Service
public class NotificationService {

    private final PictureController controller;

    public NotificationService(PictureController controller) {
        this.controller = controller;
    }

    @RabbitListener(queues = RabbitConfig.QUEUE_DONE)
    public void onDone(byte[] message) {
        // message is the picture ID as UTF-8 string
        long pictureId = Long.parseLong(new String(message).trim());
        controller.notifyFrontend(pictureId);
    }
}
