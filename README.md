# simd-fast-esp32s3

A SIMD-accelerated implementation of the FAST corner detector algorithm. Achieves roughly double the performance of a naive implementation, processing about 11.2MP/s on an ESP32-S3 clocked at 240MHz, which is enough to process a VGA (640x480) stream at 30fps.
