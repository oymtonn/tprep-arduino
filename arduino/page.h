const char PAGE_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
  <title>Roll Steady</title>
  <style>
    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }

    body {
      margin: 0;
      padding: 40px 24px;
      background: #fff;
      color: #111;
      font: 400 16px/1.4 -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      -webkit-user-select: none;
      user-select: none;
    }

    /* ---- header ---- */

    h1 {
      margin: 0;
      font-size: 17px;
      font-weight: 600;
      text-align: center;
    }
    .sub {
      margin: 6px 0 40px;
      font-size: 13px;
      color: #999;
      text-align: center;
    }

    /* ---- readouts ---- */

    .value { text-align: center; }
    .value b {
      display: block;
      font-size: 40px;
      font-weight: 500;
      letter-spacing: -0.02em;
      font-variant-numeric: tabular-nums;
    }
    .value span {
      display: block;
      margin-top: 4px;
      font-size: 12px;
      color: #999;
    }

    /* live speed, above the slider */
    .value.now { margin-bottom: 44px; }

    /* target speed, below the slider */
    .value.target { margin-top: 26px; margin-bottom: 10px;}
    .value.target b { font-size: 26px; color: #000; }

    /* editable target box */
    .value.target input {
      display: block;
      width: 100px;
      margin: 0 auto;
      padding: 2px 0;
      border: 0;
      border-bottom: 2px solid #111;
      background: none;
      font: inherit;
      font-size: 26px;
      font-weight: 500;
      text-align: center;
      color: #000;
      font-variant-numeric: tabular-nums;
      -moz-appearance: textfield;
    }
    .value.target input:focus {
      outline: none;
      border-bottom-color: #999;
    }
    .value.target input::-webkit-outer-spin-button,
    .value.target input::-webkit-inner-spin-button {
      -webkit-appearance: none;
      margin: 0;
    }

    /* ---- slider ---- */

    .labels {
      display: flex;
      justify-content: space-between;
      margin: 0 14px 12px;
      font-size: 12px;
      color: #999;
      font-variant-numeric: tabular-nums;
    }
    .labels b { font-weight: 600; color: #111; }

    .slider { position: relative; height: 28px; }

    .rail, .fill {
      position: absolute;
      top: 12px;
      height: 4px;
      border-radius: 2px;
    }
    .rail { left: 0; right: 0; background: #e4e4e4; }
    .fill { left: 0; width: 14px; background: #111; }

    .dots {
      position: absolute;
      left: 14px; right: 14px; top: 12px;
      display: flex;
      justify-content: space-between;
    }
    .dots i {
      width: 4px; height: 4px;
      border-radius: 50%;
      background: #c9c9c9;
    }
    .dots i.on { background: #fff; }

    input[type=range] {
      -webkit-appearance: none;
      appearance: none;
      position: absolute;
      inset: 0;
      width: 100%;
      margin: 0;
      background: none;
      outline: none;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 28px; height: 28px;
      border-radius: 50%;
      background: #fff;
      border: 4px solid #111;
    }
    input[type=range]::-moz-range-thumb {
      width: 20px; height: 20px;
      border-radius: 50%;
      background: #fff;
      border: 4px solid #111;
    }

    /* ---- buttons ---- */

    .btn {
      display: block;
      width: 100%;
      padding: 17px;
      border: 0;
      border-radius: 8px;
      margin-bottom:20px;
      font: inherit;
      font-weight: 500;
      background: #111;
      color: #fff;
      box-shadow: 0 4px 0 #000;
      transform: translateY(0);
      transition: transform 70ms ease, box-shadow 70ms ease;
    }
    .btn:active {
      transform: translateY(4px);
      box-shadow: 0 0 0 #000;
    }

    .ghost {
      background: #fff;
      color: #111;
      border: 2px solid #111;
      padding: 15px;
      box-shadow: 0 4px 0 #111;
      letter-spacing: 0.08em;
    }
    .ghost:active { background: #111; color: #fff; }

    /* ---- layout ---- */

    .apply-row { margin-top: 36px; }

    .halt-row {
      margin-top: 50px;
      padding-top: 28px;
      border-top: 1px solid #eee;
    }
  </style>
</head>
<body ontouchstart="">

  <h1>Roll Steady</h1>

  <div class="value now">
    <span id="status">Current speed</span>
    <b id="cur">0.0</b>
    <p>mph</p>
  </div>

  <div class="labels" id="lb">
    <span>0</span><span>1</span><span>2</span>
    <span>3</span><span>4</span><span>5</span>
  </div>

  <div class="slider">
    <div class="rail"></div>
    <div class="fill" id="f"></div>
    <div class="dots" id="d">
      <i></i><i></i><i></i><i></i><i></i><i></i>
    </div>
    <input type="range" id="r" min="0" max="5" step="0.5" value="1">
  </div>

  <div class="value target">
    <span>Target speed</span>
    <input type="number" id="o" min="0" max="5" step="0.1" value="1" inputmode="decimal">
    <p>mph</p>
  </div>

  <div class="apply-row">
    <button class="btn" onclick="send(o.value)">Apply</button>
  </div>

  <div class="halt-row">
    <button class="btn ghost" onclick="send(0)">STOP</button>
  </div>

  <script>
    var r = document.getElementById('r');
    var o = document.getElementById('o');

    // the real target, to 0.1 mph. the slider is only a coarse view of it.
    var target = 1;

    // repaint the fill, dots and labels from the slider's own position
    function drawBar() {
      var v = +r.value, p = (v - r.min) / (r.max - r.min);
      document.getElementById('f').style.width =
        'calc(14px + (100% - 28px) * ' + p + ')';
      var d = document.getElementById('d').children,
          l = document.getElementById('lb').children;
      for (var i = 0; i < d.length; i++) {
        d[i].className = i <= v ? 'on' : '';
        l[i].innerHTML = i == v ? '<b>' + i + '</b>' : i;
      }
    }

    // store a new target and park the slider on the nearest half step
    function setTarget(v) {
      if (isNaN(v)) return;
      v = Math.min(5, Math.max(0, v));
      target = Math.round(v * 10) / 10;       // 0.1 grid
      r.value = Math.round(target * 2) / 2;   // slider snaps to 0.5
      drawBar();
    }

    var synced = false;

    // the Arduino replies with "target_mph,current_mph"  e.g. "1.3,0.8"
    function apply(t) {
      var p = t.split(',');
      document.getElementById('cur').textContent = p[1];
      document.getElementById('status').textContent = 'Current speed';

      // only adopt the Arduino's target once, on first load
      if (!synced) {
        setTarget(parseFloat(p[0]));
        o.value = target;
        synced = true;
      }
    }

    function fail() {
      document.getElementById('status').textContent = 'No response';
    }

    function send(v) {
      setTarget(parseFloat(v));
      o.value = target;
      fetch('/set?v=' + target).then(function (x) { return x.text(); })
        .then(apply).catch(fail);
    }

    function poll() {
      fetch('/state').then(function (x) { return x.text(); })
        .then(apply).catch(fail);
    }

    // slider dragged -> coarse target, box follows
    r.addEventListener('input', function () {
      setTarget(+r.value);
      o.value = target;
    });

    // box typed -> fine target, slider follows but the box is left alone
    o.addEventListener('input', function () {
      setTarget(parseFloat(o.value));
    });

    // on leaving the box, show the stored target (not the slider's rounding)
    o.addEventListener('blur', function () { o.value = target; });

    o.addEventListener('keydown', function (e) {
      if (e.key === 'Enter') { o.blur(); send(target); }
    });

    o.value = target;
    drawBar();
    poll();
    setInterval(poll, 400);
  </script>

</body>
</html>
)rawliteral";