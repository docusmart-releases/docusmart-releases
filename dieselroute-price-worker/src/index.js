// Deployment-Trigger nach Korrektur des Cloudflare-Stammverzeichnisses.
// DieselRoute: serverseitiger Live-Preis-Proxy zur TankPuls REST-API.
// Keine API-Schlüssel erforderlich für anonyme Nutzung (Rate-Limits beachten).
const ALLOWED_ORIGIN = 'https://docusmart-releases.github.io';
const UPSTREAM = 'https://api.tankpuls.de/api/stations/search';
const json = (data, status, extra = {}) => new Response(JSON.stringify(data), {
  status,
  headers: {'Content-Type':'application/json; charset=utf-8', 'Cache-Control':'no-store', ...extra}
});
export default {
  async fetch(request) {
    const url = new URL(request.url);
    const origin = request.headers.get('Origin');
    const cors = {
      'Access-Control-Allow-Origin': ALLOWED_ORIGIN,
      'Access-Control-Allow-Methods': 'GET, OPTIONS',
      'Access-Control-Allow-Headers': 'Accept',
      'Vary': 'Origin'
    };
    // Host should only be invoked by the published DieselRoute webpage.
    if(origin && origin !== ALLOWED_ORIGIN) return json({error:'origin_not_allowed'},403,cors);
    if(request.method==='OPTIONS') return new Response(null,{status:204,headers:cors});
    if(request.method!=='GET') return json({error:'method_not_allowed'},405,cors);
    if(url.pathname==='/health') return json({ok:true,service:'dieselroute-prices',source:'TankPuls'},200,cors);
    if(url.pathname!=='/api/stations/search') return json({error:'not_found'},404,cors);
    const lat=Number(url.searchParams.get('lat'));
    const lng=Number(url.searchParams.get('lng'));
    const rad=Number(url.searchParams.get('rad'));
    const fuel=url.searchParams.get('fuel') || 'diesel';
    const limit=Number(url.searchParams.get('limit')||'100');
    if(!url.searchParams.has('lat')||!url.searchParams.has('lng')||!url.searchParams.has('rad')||
       !Number.isFinite(lat)||Math.abs(lat)>90||!Number.isFinite(lng)||Math.abs(lng)>180||
       !Number.isFinite(rad)||rad<=0||rad>50||!['diesel','e5','e10'].includes(fuel)||
       !Number.isInteger(limit)||limit<1||limit>500){
      return json({error:'invalid_parameters'},400,cors);
    }
    const upstream=new URL(UPSTREAM);
    for(const [k,v] of Object.entries({lat,lng,rad,fuel,limit})) upstream.searchParams.set(k,String(v));
    const timeout=new AbortController();
    const timer=setTimeout(()=>timeout.abort(),12000);
    try {
      const response=await fetch(upstream.toString(), {
        headers:{'Accept':'application/json'},
        signal:timeout.signal,
        cf:{cacheTtl:30,cacheEverything:true}
      });
      const contentType=response.headers.get('content-type')||'';
      const body=await response.text();
      if(!contentType.includes('application/json'))return json({error:'invalid_upstream_response'},502,cors);
      let data;
      try{data=JSON.parse(body)}catch{return json({error:'invalid_upstream_json'},502,cors)}
      if(!response.ok)return json({error:'upstream_error',status:response.status,detail:data.error||null},
        response.status===429?429:502,cors);
      if(!Array.isArray(data.items))return json({error:'missing_stations'},502,cors);
      return new Response(JSON.stringify({items:data.items,count:data.count||data.items.length}), {
        status:200,
        headers:{'Content-Type':'application/json; charset=utf-8','Cache-Control':'public, max-age=20',
          'X-Data-Source':'MTS-K via tankerkoenig.de / TankPuls',...cors}
      });
    }catch(e){return json({error:e.name==='AbortError'?'upstream_timeout':'upstream_unavailable'},503,cors)}
    finally{clearTimeout(timer)}
  }
};
