# DecibelServer

## Dependencies
```bash
pip install -r requirements.txt
```

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


