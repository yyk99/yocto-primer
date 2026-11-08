# Server library only, with the plain encodings (raw, copyrect, hextile, rre).
# No zlib/jpeg/png, no TLS or websockets, no SDL, no threads: they cost RAM and
# CPU on a 200 MHz ARM920T, and the demo reaches the server over an ssh tunnel.
PACKAGECONFIG = ""
