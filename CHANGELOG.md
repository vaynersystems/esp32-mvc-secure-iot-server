# Changelog

## v0.7.0

### Summary
Security hardening across sign-in, passwords, backups, HTTPS and access control. A new captive portal for first-time setup. Faster, more stable HTTPS. MQTT over TLS no longer disrupts the web server. 15 MB SPIFFS on 32 MB S3 boards. Reworked dark theme, and restart handling that waits for the device to come back.

### Security
- **Sign-in tokens:**
  - A leaked token can no longer be used as a password.
  - The token signing key is now derived from the device's private key instead of being fixed.
  - The signature check now takes the same time whether it matches or not.
  - Each token carries a per-user session id.
  - Tokens of disabled users are refused.
- **Passwords:**
  - Passwords are stored as salted PBKDF2-SHA256 hashes (4096 rounds), replacing unsalted hashes.
  - Username lookup now matches exactly; before, a login with any username could match.
- **Login throttling:** failed logins are limited per IP, plus a global limit of 10 failures per 60 s.
- **Backups:** encrypted with an authenticated key (backup/restore v2), so users can still log in after a restore. Restore checks the user list and gives up if the upload stalls.
- **HTTPS:**
  - New certificates use ECDSA P-256 with the proper fields (SAN, leaf extensions, random serial).
  - Old self-signed RSA certificates are replaced automatically.
  - Sign-in only works over HTTPS.
- **Access control:**
  - Non-admins can only read site files under `/W/`; `..` and `//` paths are refused.
  - Internal folders (`/INT`, `/TMP`, `/LOG`) are blocked.
  - The file list, uploads, logs, users and config are admin-only, and drive numbers are range-checked.
- **Secrets stay on the device:** Wi-Fi and MQTT passwords and other secrets are removed before config goes to the browser, and merged back in on save.
- **Users page:**
  - Usernames must be 3–32 characters and passwords 8–64.
  - Roles are checked.
  - The last enabled admin can't be deleted or disabled.
  - Values are escaped, so user data can't inject HTML.
- **Request limits (server library):**
  - Headers are capped (40 headers / 8 KB); requests over the limit get 431.
  - WebSocket messages are capped at 4 KB, with proper frame parsing.
  - Request bodies are drained with a time limit, so a stalled upload can't freeze the server.
  - Socket timeouts were added.
- **WebSockets:**
  - Unauthenticated connections are redirected.
  - Message fields are type-checked.
  - Clients can no longer broadcast arbitrary messages.
- **Firmware update (OTA):**
  - Images are checked against the partition size.
  - A stalled upload times out after 10 s.
  - A memory-free bug and a bogus version check were fixed.

### Wi-Fi and captive portal
- New captive portal with its own DNS responder. The access point name is based on the MAC address, and the DHCP server hands out router and DNS settings.
- A plain HTTP landing page points to HTTPS. TLS connections for unknown hostnames are filtered out early.
- **Static IP fix:** the static IP setting was only read in debug builds, so static IP configurations were ignored.
- Join timeout is 10 s (was 15 s; override with `-DWIFI_JOIN_TIMEOUT_MS`), with step-by-step join logging.
- Open (no-password) access point option added in config.
- Captive-portal DNS query logging is off by default. Turn it on with **Log Portal DNS Queries** on the config page's Logging section (`system.logging.dns`).

### Performance and stability
- **Faster connections:**
  - HTTP keep-alive with `Content-Length`.
  - TLS session resumption.
  - ECDSA certificates.
  - A cache of up to 64 served files, cleared on upload.
- **Connection slots:** 8 on PSRAM boards (4 otherwise). When slots run out, idle keep-alive connections are closed first, then the oldest WebSocket.
- **TLS handshake:** the server waits for the whole ClientHello (works around how lwIP peeks at data). Handshakes time out after 1 s, slow clients are dropped, and `SSL_accept` errors are logged in detail when `HTTPS_LOGLEVEL` is 1 or higher.
- **Freezes fixed:**
  - LCD loop no longer runs under the shared lock.
  - `getTime()` no longer waits.
  - `readLine` bug fixed.
  - DS18B20 temperature reads no longer block.
  - The scheduler skips while the clock isn't set.
  - Device and schedule loops wait until setup is finished.
- TLS memory now comes from PSRAM, and the LCD turns itself off if it isn't connected.
- Config sections are parsed with a buffer sized to the file, and errors are logged.
- **MQTT over TLS no longer breaks the web server:** while the broker was unreachable, the client retried every 200 ms with two TLS handshakes per try, which made pages load partially.
  - Failed connects back off from 5 s up to 5 minutes, with one log line per attempt.
  - One handshake per attempt, and the TLS handshake timeout is 10 s instead of 120 s.
  - At most 50 messages wait for the broker; past that the oldest are dropped.
  - The publish queue is now locked between the device and MQTT tasks; before, adding and sending at the same time could corrupt memory. A publish that fails partway resends only the unsent messages.
- HTTPS server library logging is off (`HTTPS_LOGLEVEL=0`), and a stray `Content type:` print on uploads was removed.

### Web UI
- **Dark theme reworked:**
  - Proper light and dark palettes, plus a "Match device setting" option.
  - Applied on every page before it draws, and remembered per browser.
  - Hard-coded colours replaced, including in the logs table.
- **Restart spinner:** after Save or Reset it now waits for the device to come back, then reloads the page. This covers the config page, the devices page (where it used to hang forever) and the Reset button (which used to not wait at all).
- **Sign-in:**
  - Safe return path after login.
  - Cookie marked `Secure` and `SameSite`.
  - Spinner stays up through the redirect.
  - Setup-code field shown when setup is required.
- WebSocket reconnects with backoff and closes when the page is left.
- **Users page:**
  - Server errors are shown instead of failing silently.
  - Form limits match the server.
  - The "Edit undefined" title is fixed.
- Config page panels scroll properly, the menu falls back when the icon font is missing, and password fields were fixed.

### Build
- `espressif32@6.13.0` pinned for all environments; newer platforms dropped OpenSSL.
- The 16 MB S3 environment uses `esp32-s3-devkitc-1` with flash and PSRAM overrides.
- The 32 MB S3 environment has its own partition table (`partitionss3-32MB.csv`): two 4 MB app slots and 15 MB SPIFFS, up from 3.4 MB. SPIFFS can't go past 16 MB. Moving to it erases SPIFFS (users, config, logs); back up first.

### Known issues
- DHCP fails on some networks; use a static IP as a workaround.
- A slow TLS handshake is bounded but can still take about 2 minutes in the worst case.
- OTA images aren't signature-checked; that needs Secure Boot.
- `printf(fileInfo)` format-string bug in the external `esp32-filesystem` library.
