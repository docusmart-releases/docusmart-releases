# DieselRoute – Preis-Worker

Serverseitige, begrenzte Abfrage der dokumentierten öffentlichen TankPuls-API.
Der Worker ist vorbereitet, aber noch **nicht bereitgestellt**. Er darf erst als
funktionierend bezeichnet werden, wenn eine echte Workers-URL und eine erfolgreiche
Testantwort vorliegen.

## Bereitstellen

1. Cloudflare Workers-Konto anmelden.
2. Im Ordner `dieselroute-price-worker` einmal `npx wrangler deploy` ausführen.
   Die Cloudflare-Anmeldung ist dafür erforderlich. Alternativ kann das Worker-Script
   im Cloudflare-Dashboard eingerichtet werden.
3. Test: `https://<worker>.workers.dev/health` muss `ok:true` zurückgeben.
4. Test: `https://<worker>.workers.dev/api/stations/search?lat=52.52&lng=13.405&rad=5&fuel=diesel&limit=5`
   soll ein Array `items` zurückgeben.
5. Erst dann in der DieselRoute-Webseite `getStations()` von öffentlichen Proxy-Diensten
   auf die verifizierte Worker-URL umstellen und produktiv testen.

Hinweise: Öffentliche API ohne Schlüssel hat dokumentierte Limits (60 Listen-Anfragen
pro Minute und IP); bei vielen Nutzern/Korridoren weitere Drosselung und Caching
vorsehen. Keine Standort-Hintergrundüberwachung, kein Persistieren von Koordinaten.
Quellenangabe: MTS-K über tankerkoenig.de, CC BY 4.0 · TankPuls.

Docs: https://api.tankpuls.de/docs
