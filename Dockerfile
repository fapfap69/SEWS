# Dockerfile
FROM alpine:3.14 as builder

RUN apk add --no-cache \
    gcc \
    musl-dev \
    make \
    openssl-dev

WORKDIR /app
COPY . .
RUN make

FROM alpine:3.14

RUN apk add --no-cache \
    libssl1.1

WORKDIR /app
COPY --from=builder /app/sews /app/sews
COPY www/ /app/www/

EXPOSE 8080

CMD ["./sews"]
