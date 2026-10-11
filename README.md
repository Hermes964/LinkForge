# LinkForge

Custom Android-to-Windows remote desktop and remote-control system.

**Status:** Prototype — not production-ready. This project is in early development (Stage 0/1).

## Project Structure

```
protocol/           Shared packet definitions and serialization
windows/            Windows host, capture, encoder, input and diagnostics
android/            Kotlin Android application and decoder
server/             Future coordination API, signalling and relay
tests/              Unit and integration tests
docs/               Architecture, protocol, security and development guides
```

## Development

- Windows host: C++20, CMake, Visual Studio 2026, D3D11, AMD AMF
- Android client: Kotlin, Jetpack Compose, MediaCodec
- Shared protocol: Versioned binary protocol with explicit serialization
- Network: UDP (local network prototype), HTTPS/WebSocket (future)

## Documentation

- `docs/protocol-v1.md` — Wire format and validation rules
- `docs/architecture.md` — Component responsibilities and data flow
- `docs/threat-model.md` — Assets, trust boundaries, risks and mitigations
- `docs/build-windows.md` — Windows dependencies and commands
- `docs/build-android.md` — Android build instructions

## License

See repository for details.
