package main

import (
	"crypto/rand"
	"encoding/base64"
	"encoding/binary"
	"fmt"
	"io"
	"log"
	"net"
	"net/http"
	"os"
	"strconv"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/pkg/errors"
	"golang.org/x/crypto/chacha20poly1305"
)

const (
	port                     = 8081
	serverAddress            = "127.0.0.1"
	serverPort               = 40000
	keyBytes                 = 32
	authBytes                = 16
	connectTokenExpiry       = 45
	connectTokenBytes        = 2048
	connectTokenPrivateBytes = 1024
	userDataBytes            = 256
	timeoutSeconds           = 5
	versionInfo              = "NETCODE 1.02\x00"
	verboseError             = true
	addressIPV4              = 1
	addressIPV6              = 2
)

var (
	stdoutLogger = log.New(os.Stdout, "yojimbo-matcher: ", log.Llongfile)
	stderrLogger = log.New(os.Stderr, "yojimbo-matcher: ", log.Llongfile)
	privateKey   = []byte{
		0x60, 0x6a, 0xbe, 0x6e, 0xc9, 0x19, 0x10, 0xea,
		0x9a, 0x65, 0x62, 0xf6, 0x6f, 0x2b, 0x30, 0xe4,
		0x43, 0x71, 0xd6, 0x2c, 0xd1, 0x99, 0x27, 0x26,
		0x6b, 0x3c, 0x60, 0xf4, 0xb7, 0x15, 0xab, 0xa1,
	}
)

func writeAddresses(buffer []byte, addresses []net.UDPAddr) int {
	binary.LittleEndian.PutUint32(buffer[0:], (uint32)(len(addresses)))
	offset := 4
	for _, addr := range addresses {
		ipv4 := addr.IP.To4()
		port := addr.Port
		if ipv4 != nil {
			buffer[offset] = addressIPV4
			buffer[offset+1] = ipv4[0]
			buffer[offset+2] = ipv4[1]
			buffer[offset+3] = ipv4[2]
			buffer[offset+4] = ipv4[3]
			buffer[offset+5] = (byte)(port & 0xFF)
			buffer[offset+6] = (byte)(port >> 8)
		} else {
			buffer[offset] = addressIPV6
			copy(buffer[offset+1:], addr.IP)
			buffer[offset+17] = (byte)(port & 0xFF)
			buffer[offset+18] = (byte)(port >> 8)
		}
		offset += 19
	}
	return offset
}

type connectTokenPrivate struct {
	clientID          uint64
	TimeoutSeconds    int32
	ServerAddresses   []net.UDPAddr
	ClientToServerKey [keyBytes]byte
	ServerToClientKey [keyBytes]byte
	UserData          [userDataBytes]byte
}

func newConnectTokenPrivate(clientID uint64, serverAddresses []net.UDPAddr, timeoutSeconds int32, userData []byte, clientToServerKey []byte, serverToClientKey []byte) *connectTokenPrivate {
	connectTokenPrivate := &connectTokenPrivate{}
	connectTokenPrivate.clientID = clientID
	connectTokenPrivate.TimeoutSeconds = timeoutSeconds
	connectTokenPrivate.ServerAddresses = serverAddresses
	copy(connectTokenPrivate.UserData[:], userData)
	copy(connectTokenPrivate.ClientToServerKey[:], clientToServerKey)
	copy(connectTokenPrivate.ServerToClientKey[:], serverToClientKey)
	return connectTokenPrivate
}

func (token *connectTokenPrivate) Write(buffer []byte) {
	binary.LittleEndian.PutUint64(buffer[0:], token.clientID)
	binary.LittleEndian.PutUint32(buffer[8:], (uint32)(token.TimeoutSeconds))
	addressBytes := writeAddresses(buffer[12:], token.ServerAddresses)
	copy(buffer[12+addressBytes:], token.ClientToServerKey[:])
	copy(buffer[12+addressBytes+keyBytes:], token.ServerToClientKey[:])
	copy(buffer[12+addressBytes+keyBytes*2:], token.UserData[:])
}

type connectToken struct {
	protocolID        uint64
	CreateTimestamp   uint64
	ExpireTimestamp   uint64
	Sequence          uint64
	PrivateData       *connectTokenPrivate
	TimeoutSeconds    int32
	ServerAddresses   []net.UDPAddr
	ClientToServerKey [keyBytes]byte
	ServerToClientKey [keyBytes]byte
	PrivateKey        [keyBytes]byte
}

func newConnectToken(clientID uint64, serverAddresses []net.UDPAddr, protocolID uint64, expireSeconds uint64, timeoutSeconds int32, userData []byte, privateKey []byte) (*connectToken, error) {
	connectToken := &connectToken{}
	connectToken.protocolID = protocolID
	connectToken.CreateTimestamp = uint64(time.Now().Unix())
	if expireSeconds >= 0 {
		connectToken.ExpireTimestamp = connectToken.CreateTimestamp + expireSeconds
	} else {
		connectToken.ExpireTimestamp = 0xFFFFFFFFFFFFFFFF
	}
	connectToken.TimeoutSeconds = timeoutSeconds
	connectToken.ServerAddresses = serverAddresses
	err := fillWithRandomBytes(connectToken.ClientToServerKey[:])
	if err != nil {
		return nil, errors.Wrap(err, "failed to fill client to server key with random bytes")
	}
	err = fillWithRandomBytes(connectToken.ServerToClientKey[:])
	if err != nil {
		return nil, errors.Wrap(err, "failed to fill server to client key with random bytes")
	}
	copy(connectToken.PrivateKey[:], privateKey[:])
	connectToken.PrivateData = newConnectTokenPrivate(clientID, serverAddresses, timeoutSeconds, userData, connectToken.ClientToServerKey[:], connectToken.ServerToClientKey[:])
	return connectToken, nil
}

func fillWithRandomBytes(buf []byte) error {
	_, err := rand.Read(buf)
	if err != nil {
		return err
	}
	return nil
}

func encryptAEAD(message []byte, additional []byte, nonce []byte, key []byte) error {
	aead, err := chacha20poly1305.NewX(key)
	if err != nil {
		return errors.Wrap(err, "failed to create cipher")
	}

	// Encrypt the message and append the authentication tag.
	aead.Seal(message[:0], nonce, message, additional)

	return nil
}

func (token *connectToken) Write(buffer []byte) error {
	copy(buffer, versionInfo)
	binary.LittleEndian.PutUint64(buffer[13:], token.protocolID)
	binary.LittleEndian.PutUint64(buffer[21:], token.CreateTimestamp)
	binary.LittleEndian.PutUint64(buffer[29:], token.ExpireTimestamp)
	nonce := make([]byte, 24)
	err := fillWithRandomBytes(nonce)
	if err != nil {
		return errors.Wrap(err, "failed to fill nonce with random bytes")
	}
	copy(buffer[37:], nonce[:])
	token.PrivateData.Write(buffer[61:])
	additional := make([]byte, 13+8+8)
	copy(additional, versionInfo[0:13])
	binary.LittleEndian.PutUint64(additional[13:], token.protocolID)
	binary.LittleEndian.PutUint64(additional[21:], token.ExpireTimestamp)
	err = encryptAEAD(buffer[61:61+connectTokenPrivateBytes-authBytes], additional[:], nonce[:], token.PrivateKey[:])
	if err != nil {
		return errors.Wrap(err, "failed to encrypt message")
	}
	binary.LittleEndian.PutUint32(buffer[connectTokenPrivateBytes+61:], (uint32)(token.TimeoutSeconds))
	offset := writeAddresses(buffer[1024+61+4:], token.ServerAddresses)
	copy(buffer[1024+61+4+offset:], token.ClientToServerKey[:])
	copy(buffer[1024+61+4+offset+keyBytes:], token.ServerToClientKey[:])
	return nil
}

func generateConnectToken(clientID uint64, serverAddresses []net.UDPAddr, protocolID uint64, expireSeconds uint64, timeoutSeconds int32, userData []byte, privateKey []byte) ([]byte, error) {
	connectToken, err := newConnectToken(clientID, serverAddresses, protocolID, expireSeconds, timeoutSeconds, userData, privateKey)
	if err != nil {
		return nil, errors.Wrap(err, "failed to create connect token")
	}
	buffer := make([]byte, connectTokenBytes)
	err = connectToken.Write(buffer)
	if err != nil {
		return nil, errors.Wrap(err, "failed to serialize connect token")
	}
	return buffer, nil
}

func writeError(w http.ResponseWriter, err error, statusCode int) {
	stderrLogger.Printf("%+v\n", err)
	errMessage := "An error occured on the server while processing the request"
	if verboseError {
		errMessage = err.Error()
	}
	w.Header().Set("Content-Type", "text/plain; charset=utf-8")
	w.Header().Set("X-Content-Type-Options", "nosniff")
	w.WriteHeader(statusCode)
	fmt.Fprint(w, errMessage)
}

func matchHandler(w http.ResponseWriter, r *http.Request) {

	clientID, err := strconv.ParseUint(chi.URLParam(r, "clientID"), 10, 64)
	if err != nil {
		writeError(w, fmt.Errorf("Unable to parse clientID: %s", chi.URLParam(r, "clientID")), http.StatusBadRequest)
		return
	}
	protocolID, err := strconv.ParseUint(chi.URLParam(r, "protocolID"), 10, 64)
	if err != nil {
		writeError(w, fmt.Errorf("Unable to parse protocolID: %s", chi.URLParam(r, "protocolID")), http.StatusBadRequest)
		return
	}

	serverAddresses := make([]net.UDPAddr, 1)
	serverAddresses[0] = net.UDPAddr{IP: net.ParseIP(serverAddress), Port: serverPort}

	userData := make([]byte, userDataBytes)
	connectToken, err := generateConnectToken(clientID, serverAddresses, protocolID, connectTokenExpiry, timeoutSeconds, userData, privateKey)
	if err != nil {
		writeError(w, errors.Wrap(err, "Failed to generate connect token"), http.StatusInternalServerError)
		return
	}
	connectTokenBase64 := base64.StdEncoding.EncodeToString(connectToken)
	w.Header().Set("Content-Type", "application/text")
	_, err = io.WriteString(w, connectTokenBase64)
	if err != nil {
		writeError(w, errors.Wrap(err, "Failed to write response"), http.StatusInternalServerError)
		return
	}
	stderrLogger.Printf("Matched client %.16x to %s:%d\n", clientID, serverAddress, serverPort)
}

func main() {
	stderrLogger.Printf("Started matchmaker on port %d\n", port)

	router := chi.NewRouter()
	router.Get("/match/{protocolID:[0-9]+}/{clientID:[0-9]+}", matchHandler)

	err := http.ListenAndServe(":"+strconv.Itoa(port), router)
	if err != nil {
		stderrLogger.Fatalf("%+v\n", err)
	}
}
