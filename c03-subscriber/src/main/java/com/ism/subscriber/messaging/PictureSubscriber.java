package com.ism.subscriber.messaging;

import org.springframework.amqp.rabbit.annotation.RabbitListener;
import org.springframework.amqp.rabbit.core.RabbitTemplate;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.*;
import org.springframework.stereotype.Service;
import org.springframework.web.client.RestTemplate;

import java.io.*;
import java.nio.file.*;
import java.util.UUID;

@Service
public class PictureSubscriber {

    private static final String QUEUE_IN   = "ism.pictures.in";
    private static final String EXCHANGE   = "ism.topic";
    private static final String RK_DONE   = "picture.done";

    @Value("${mpi.hosts:localhost}") private String mpiHosts;
    @Value("${mpi.np:2}")            private int    mpiNp;
    @Value("${c05.rest.url}")        private String c05Url;

    private final RabbitTemplate rabbit;
    private final RestTemplate   http = new RestTemplate();

    public PictureSubscriber(RabbitTemplate rabbit) {
        this.rabbit = rabbit;
    }

    @RabbitListener(queues = QUEUE_IN)
    public void onMessage(byte[] raw) throws Exception {
        // ── 1. Parse packed message: header lines + BMP bytes ──
        int headerEnd = 0;
        int newlines = 0;
        while (newlines < 4 && headerEnd < raw.length) {
            if (raw[headerEnd++] == '\n') newlines++;
        }
        String headerStr = new String(raw, 0, headerEnd).trim();
        String[] parts   = headerStr.split("\n", 4);
        String filename  = parts[0];
        String keyHex    = parts[1];
        String operation = parts[2];
        String mode      = parts[3];

        byte[] bmpBytes  = new byte[raw.length - headerEnd];
        System.arraycopy(raw, headerEnd, bmpBytes, 0, bmpBytes.length);

        // ── 2. Write BMP to shared /mpi-work dir ──
        String jobId    = UUID.randomUUID().toString().substring(0, 8);
        Path   workDir  = Path.of("/mpi-work");
        Path   inFile   = workDir.resolve(jobId + "_in.bmp");
        Path   outFile  = workDir.resolve(jobId + "_out.bmp");
        Files.write(inFile, bmpBytes);

        // ── 3. Launch MPI (Runtime.exec → native ELF64) ──
        String cmd = String.format(
            "mpirun -np %d --host %s /opt/aes_enc %s %s %s %s %s",
            mpiNp, mpiHosts, operation, mode, keyHex,
            inFile.toAbsolutePath(), outFile.toAbsolutePath()
        );
        System.out.println("[C03] Executing: " + cmd);

        Process proc = Runtime.getRuntime().exec(new String[]{"bash", "-c", cmd});
        int exit = proc.waitFor();

        // Stream MPI stdout/stderr to our logs
        new String(proc.getInputStream().readAllBytes())
            .lines().forEach(l -> System.out.println("[MPI] " + l));
        new String(proc.getErrorStream().readAllBytes())
            .lines().forEach(l -> System.err.println("[MPI ERR] " + l));

        if (exit != 0) {
            System.err.println("[C03] MPI process exited with code " + exit);
            return;
        }

        // ── 4. POST result BMP to C05 ──
        byte[] outBytes = Files.readAllBytes(outFile);
        HttpHeaders headers = new HttpHeaders();
        headers.setContentType(MediaType.APPLICATION_OCTET_STREAM);
        headers.set("X-Filename",  filename);
        headers.set("X-Operation", operation);
        headers.set("X-Mode",      mode);

        ResponseEntity<String> resp = http.exchange(
            c05Url + "/picture",
            HttpMethod.POST,
            new HttpEntity<>(outBytes, headers),
            String.class
        );

        String pictureId = resp.getBody();
        System.out.println("[C03] Stored picture id=" + pictureId);

        // ── 5. Notify C01 via "done" queue ──
        rabbit.convertAndSend(EXCHANGE, RK_DONE, pictureId.getBytes());

        // Cleanup temp files
        Files.deleteIfExists(inFile);
        Files.deleteIfExists(outFile);
    }
}
