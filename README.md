# DecibelServer

## Dependencies/Requirements
### Python Implementation
```bash
pip install -r requirements.txt
```
### C++ Implementation
A C++20 capable compiler is required. Currently, the following should be able to build the project:
- gcc >= 9
- clang >= 9
- MSVC >= 19.21
- Apple clang >= 11.0.3

#### Build Tools
- [cmake](https://cmake.org/)

#### Third Party Libraries
- [cxxopts](https://github.com/jarro2783/cxxopts)
- [fmt](https://github.com/fmtlib/fmt)
- [nlohmann_json](https://github.com/nlohmann/json)
- [websocketpp](https://github.com/zaphoyd/websocketpp)

For easy installation, these dependencies can all be installed "automatically" with [conan](https://conan.io/). Install conan with:
```bash
pip install -U conan
```
and then `cmake` will take care of installing whatever is needed.

Otherwise, these packages can be installed manually/through whatever method is preferred.

## Generating an SSL Certificate
### Local Testing
```bash
openssl req -x509 -out localhost.crt -keyout localhost.key \
  -newkey rsa:2048 -nodes -sha256 \
  -subj '/CN=localhost' -extensions EXT -config <( \
   printf "[dn]\nCN=localhost\n[req]\ndistinguished_name = dn\n[EXT]\nsubjectAltName=DNS:localhost\nkeyUsage=digitalSignature\nextendedKeyUsage=serverAuth")
```
### Deploy
- forward port 80 to server
- install [certbot](https://certbot.eff.org/instructions)

```bash
sudo apt install certbot
sudo certbot certonly --standalone
```

- fix permissions of certificate so we can run server as non-root
```bash
CERT_DIR=</path/to/cert/root> 
sudo chmod 755 -R $CERT_DIR/archive $CERT_DIR/live
```
- remove forwarding of port 80?

*reminder:* may need to change port forwarding settings on both modem and router. Xfinity modem settings can be changed [here](https://internet.xfinity.com/network/advanced-settings/portforwarding).
