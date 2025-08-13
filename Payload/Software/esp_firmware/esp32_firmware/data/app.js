(function(){
  const $ = id => document.getElementById(id);
  function set(k, v){ $(k).textContent = v; }

  const es = new EventSource('/events');
  es.addEventListener('open',  () => console.log('SSE connected'));
  es.addEventListener('error', () => console.log('SSE error/retry'));
  es.addEventListener('telemetry', e => {
    try{
      const d = JSON.parse(e.data);
      set('t',  d.temperature_c.toFixed(1));
      set('h',  d.humidity_pct.toFixed(1));
      set('p',  d.pressure_hpa.toFixed(1));
      set('ax', d.accel_g[0].toFixed(2));
      set('ay', d.accel_g[1].toFixed(2));
      set('az', d.accel_g[2].toFixed(2));
      set('up', (d.uptime_ms/1000).toFixed(0));
    }catch(err){}
  });
})();
