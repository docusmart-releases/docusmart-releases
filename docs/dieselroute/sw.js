// DieselRoute PWA: kein Offline-Cache von Preisen/Positionen (sie müssen aktuell sein).
self.addEventListener('install', () => self.skipWaiting());
self.addEventListener('activate', event => event.waitUntil(self.clients.claim()));
self.addEventListener('fetch', () => {});
