FROM golang:1.20.13 AS matcher_build

# Matcher
WORKDIR /matcher

# Copy go.mod and go.sum files to the workspace separately and download dependecies.
# Doing this separately will cache these as its own separate layer
COPY ./go.mod .
COPY ./go.sum .
RUN go mod download

# Copy the source code as the last step
COPY . .

# Build the binary
RUN CGO_ENABLED=0 go build -o matcher.bin main.go

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Then we copy and run it from a slim image
FROM alpine:3.5
WORKDIR /matcher

COPY --from=matcher_build /matcher/matcher.bin .

EXPOSE 8081

ENTRYPOINT ["/matcher/matcher.bin"]
