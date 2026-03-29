package com.ism.aes.config;

import org.springframework.amqp.core.*;
import org.springframework.amqp.rabbit.connection.ConnectionFactory;
import org.springframework.amqp.rabbit.core.RabbitTemplate;
import org.springframework.amqp.support.converter.MessageConverter;
import org.springframework.amqp.support.converter.SimpleMessageConverter;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

@Configuration
public class RabbitConfig {

    public static final String EXCHANGE   = "ism.topic";
    public static final String QUEUE_IN   = "ism.pictures.in";       // C01 → C03
    public static final String QUEUE_DONE = "ism.pictures.done";     // C03 → C01
    public static final String RK_IN      = "picture.process";
    public static final String RK_DONE    = "picture.done";

    @Bean TopicExchange exchange() {
        return new TopicExchange(EXCHANGE, true, false);
    }

    @Bean Queue queueIn()   { return QueueBuilder.durable(QUEUE_IN).build(); }
    @Bean Queue queueDone() { return QueueBuilder.durable(QUEUE_DONE).build(); }

    @Bean Binding bindingIn(Queue queueIn, TopicExchange exchange) {
        return BindingBuilder.bind(queueIn).to(exchange).with(RK_IN);
    }
    @Bean Binding bindingDone(Queue queueDone, TopicExchange exchange) {
        return BindingBuilder.bind(queueDone).to(exchange).with(RK_DONE);
    }

    @Bean MessageConverter messageConverter() { return new SimpleMessageConverter(); }

    @Bean RabbitTemplate rabbitTemplate(ConnectionFactory cf) {
        RabbitTemplate t = new RabbitTemplate(cf);
        t.setMessageConverter(messageConverter());
        return t;
    }
}
