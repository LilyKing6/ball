package main

import (
	"flag"
	"log"
	"net/http"

	"ballbattle-server/internal/server"
)

func main() {
	addr := flag.String("addr", ":8765", "服务监听地址")
	flag.Parse()

	hub := server.NewHub()

	http.HandleFunc("/ws", func(w http.ResponseWriter, r *http.Request) {
		server.ServeWSHTTP(hub, w, r)
	})

	http.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		_, _ = w.Write([]byte("ok"))
	})

	log.Printf("[BallBattle Server] listening on %s (ws: /ws, health: /health)", *addr)
	if err := http.ListenAndServe(*addr, nil); err != nil {
		log.Fatalf("server failed: %v", err)
	}
}
