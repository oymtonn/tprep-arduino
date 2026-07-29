#pragma once

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

    .value { text-align: center; margin-bottom: 44px; }
    .value b {
      display: block;
      font-size: 40px;
      font-weight: 500;
      letter-spacing: -0.02em;
      font-variant-numeric: tabular-nums;
    }
    .value span { font-size: 12px; color: #999; }

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

    .apply-row { margin-top: 56px; }

    .halt-row {
      margin-top: 96px;
      padding-top: 28px;
      border-top: 1px solid #eee;
    }
  </style>
</head>
<body ontouchstart="">

  <h1>Roll Steady</h1>
  <p class="sub">Set your target speed below</p>

  <div class="value">
    <b id="o">1</b>
    <span id="act">Target speed</span>
  </div>

  <div class="labels" id="lb">
    <span>0 mph</span><span>1 mph</span><span>2 mph</span>
    <span>3 mph</span><span>4 mph</span><span>5 mph</span>
  </div>

  <div class="slider">
    <div class="rail"></div>
    <div class="fill" id="f"></div>
    <div class="dots" id="d">
      <i></i><i></i><i></i><i></i><i></i><i></i>
    </div>
    <input type="range" id="r" min="0" max="5" step="1" value="1">
  </div>

  <div class="apply-row">
    <button class="btn" onclick="send(r.value)">Apply</button>
  </div>

  <div class="halt-row">
    <button class="btn ghost" onclick="send(0)">STOP</button>
  </div>

  <script>
    var r = document.getElementById('r');

    function draw() {
      var v = +r.value, p = (v - r.min) / (r.max - r.min);
      document.getElementById('o').textContent = v;
      document.getElementById('f').style.width =
        'calc(14px + (100% - 28px) * ' + p + ')';
      var d = document.getElementById('d').children,
          l = document.getElementById('lb').children;
      for (var i = 0; i < d.length; i++) {
        d[i].className = i <= v ? 'on' : '';
        l[i].innerHTML = i == v ? '<b>' + i + '</b>' : i;
      }
    }

    function mark(v) {
      document.getElementById('act').textContent = ""
    }

    function send(v) {
      r.value = v;
      draw();
      fetch('/set?v=' + v)
        .then(function (x) { return x.text(); })
        .then(mark)
        .catch(function () {
          document.getElementById('act').textContent = 'No response';
        });
    }

    // pull the value the Arduino is actually holding
    fetch('/state')
      .then(function (x) { return x.text(); })
      .then(function (t) { r.value = t; draw(); mark(t); })
      .catch(function () {});

    r.addEventListener('input', draw);
    draw();
  </script>

</body>
</html>
)rawliteral";
