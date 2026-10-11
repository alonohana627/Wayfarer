#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/noise.glsl"
#include "gamelib/sdf.glsl"

// Wayfarer's parallax backdrops: a soft pastel sky (sun by day, moon and
// stars at night), puffy storybook clouds, and per region a far and a near
// layer of rounded silhouettes - Hearthvale's hills and rooftops, Bellwood's
// round canopies, the mines' crystal-lit cave walls, the marsh's willows over
// misty water, Sunspire's columns and arches, Frostcrown's snowy peaks - plus
// a light layer (sun rays, mist, dust, blowing snow). Everything is computed
// from the scrolled canvas position, so the layers are seamless and stable.
//
//   color   main color (sky horizon, cloud, silhouette)  color2  second color
//           (sky top, cloud shade, silhouette highlight or snow)
//   params  x, y scroll offset (drawParallax), z layer (0 sky, 1 clouds,
//           2 far, 3 near, 4 light), w night 0..1
//   bits.x  region (0 town, 1 forest, 2 mines, 3 marsh, 4 ruins, 5 peak)

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

float h1(float x)
{
    return hashCell(vec2(x, 17.31));
}

// The top of a row of round canopies or hills: the highest circle over x.
float roundLine(float x, float period, float base, float rMin, float rMax, float seed)
{
    float best = 1e5;
    float cell = floor(x / period);
    for (int i = -2; i <= 2; ++i) {
        float c = cell + float(i);
        float r = mix(rMin, rMax, h1(c + seed));
        float cx = (c + 0.2 + 0.6 * h1(c * 1.7 + seed)) * period;
        float dx = x - cx;
        if (abs(dx) < r) {
            best = min(best, base + r * 0.35 - sqrt(r * r - dx * dx));
        }
    }
    return min(best, base + rMin * 0.2);
}

// Jagged mountains with rounded tips.
float peakLine(float x, float period, float base, float height, float seed)
{
    float cell = floor(x / period);
    float best = 1e5;
    for (int i = -1; i <= 1; ++i) {
        float c = cell + float(i);
        float hgt = height * (0.6 + 0.4 * h1(c + seed));
        float cx = (c + 0.5 + 0.3 * (h1(c * 2.3 + seed) - 0.5)) * period;
        float w = period * (0.55 + 0.25 * h1(c * 3.1 + seed));
        float dx = abs(x - cx);
        float y = base - hgt * max(1.0 - dx / w, 0.0);
        y += smoothstep(0.0, 0.15, 1.0 - dx / w) * 0.0;
        best = min(best, y);
    }
    return best + valueNoise(vec2(x * 0.02, seed)) * 14.0;
}

vec4 sky(vec2 uv, vec2 q, float night, uint region)
{
    vec3 top = sprite.color2.rgb;
    vec3 horizon = sprite.color.rgb;
    float t = smoothstep(0.0, 1.0, uv.y);
    vec3 c = mix(top, horizon, t);
    vec2 px = uv * sprite.size;
    if (region == 2u) {
        // The mines: a deep cave ceiling, no sky.
        c = mix(top, horizon, smoothstep(0.0, 1.0, uv.y) * 0.8);
        return vec4(dither(c, gl_FragCoord.xy, 1.0 / 255.0), 1.0);
    }
    // The sun (day) and the moon (night) - fixed in the sky.
    vec2 sun = vec2(sprite.size.x * 0.78, sprite.size.y * 0.2);
    float ds = length(px - sun);
    vec3 sunColor = mix(vec3(1.0, 0.97, 0.85), vec3(0.95, 0.97, 1.0), night);
    float disc = 1.0 - smoothstep(58.0, 61.0, ds);
    float halo = exp(-ds / 170.0) * mix(0.45, 0.25, night);
    c += sunColor * halo;
    vec3 discColor = night > 0.5 ? vec3(1.0, 0.98, 0.9) : vec3(1.0, 0.98, 0.88);
    if (night > 0.5) {
        // Moon craters.
        float crater = 1.0 - smoothstep(10.0, 12.0, length(px - sun - vec2(-18.0, 10.0)));
        crater = max(crater, 1.0 - smoothstep(7.0, 9.0, length(px - sun - vec2(16.0, -14.0))));
        discColor = mix(discColor, vec3(0.86, 0.85, 0.92), crater * 0.6);
    }
    c = mix(c, discColor * 1.15, disc * mix(0.85, 1.0, night));
    // Stars.
    if (night > 0.01) {
        vec2 cell = floor(px / 46.0);
        float r = hashCell(cell);
        vec2 at = (cell + vec2(hashCell(cell + 3.1), hashCell(cell + 7.7))) * 46.0;
        float tw = 0.6 + 0.4 * sin(sprite.time * (1.0 + r * 2.0) + r * 40.0);
        float star = (1.0 - smoothstep(0.8, 2.2, length(px - at))) * step(0.72, r) * tw;
        float cross = (1.0 - smoothstep(0.0, 1.0, min(abs(px.x - at.x), abs(px.y - at.y)))) * (1.0 - smoothstep(0.0, 7.0, length(px - at)))
                      * step(0.96, r);
        c += vec3(1.0, 0.95, 0.85) * (star + cross * 0.8) * night * (1.0 - t * 0.7);
    }
    return vec4(dither(c, gl_FragCoord.xy, 1.0 / 255.0), 1.0);
}

vec4 clouds(vec2 q, float night)
{
    if ((sprite.bits.x & 7u) == 2u) {
        return vec4(0.0); // no clouds underground
    }
    // Puffy clouds with flat-ish bottoms, drifting slowly.
    q.x += sprite.time * 9.0;
    float period = 620.0;
    float cell = floor(q.x / period);
    float alpha = 0.0;
    float shade = 0.0;
    for (int i = -1; i <= 1; ++i) {
        float c = cell + float(i);
        if (h1(c * 5.1) < 0.25) {
            continue;
        }
        vec2 center = vec2((c + 0.5) * period + (h1(c) - 0.5) * 200.0, 130.0 + h1(c * 3.7) * 300.0);
        float size = 60.0 + h1(c * 2.9) * 70.0;
        float d = 1e5;
        for (int k = 0; k < 5; ++k) {
            float fk = float(k);
            vec2 o = vec2((fk - 2.0) * size * 0.62, -sin(fk / 4.0 * 3.14159) * size * 0.45 + (h1(c + fk) - 0.5) * size * 0.2);
            d = min(d, length(q - center - o) - size * (0.55 + 0.25 * sin(fk / 4.0 * 3.14159)));
        }
        d = max(d, q.y - center.y - size * 0.35); // a flat bottom
        float a = 1.0 - smoothstep(-3.0, 3.0, d);
        float s = smoothstep(-size * 0.5, 0.0, q.y - center.y - size * 0.1) * a;
        alpha = max(alpha, a);
        shade = max(shade, s);
    }
    vec3 col = mix(sprite.color.rgb, sprite.color2.rgb, shade * 0.8);
    return vec4(col, alpha * mix(0.92, 0.4, night) * sprite.color.a);
}

vec4 silhouette(vec2 q, bool near, uint region, float night)
{
    vec3 main = sprite.color.rgb;
    vec3 second = sprite.color2.rgb;
    float H = sprite.size.y;
    float y = 1e5;     // the top of the silhouette at q.x
    float y2 = 1e5;    // a second line (rooftops, columns) in front
    if (region == 0u) {
        y = roundLine(q.x, near ? 520.0 : 380.0, H * (near ? 0.72 : 0.62), near ? 160.0 : 120.0, near ? 300.0 : 230.0, 1.0);
        if (!near) {
            // Distant rooftops between the hills.
            float cell = floor(q.x / 150.0);
            float cx = (cell + 0.5) * 150.0;
            float house = h1(cell * 4.1);
            if (house > 0.45) {
                float roofTop = H * 0.6 - 60.0 - house * 40.0;
                float roof = max(roofTop + abs(q.x - cx) * 0.7, 0.0);
                float wall = abs(q.x - cx) < 34.0 ? roofTop + 30.0 : 1e5;
                y2 = min(roof, wall);
                y2 = abs(q.x - cx) < 46.0 ? y2 : 1e5;
            }
        }
    } else if (region == 1u) {
        y = roundLine(q.x, near ? 300.0 : 210.0, H * (near ? 0.62 : 0.5), near ? 120.0 : 80.0, near ? 210.0 : 150.0, 7.0);
    } else if (region == 2u) {
        // Cave wall: a rocky rim from below and stalactites from above.
        y = H * (near ? 0.68 : 0.55) + valueNoise(vec2(q.x * 0.01, 2.0)) * 120.0 - abs(sin(q.x * 0.013)) * 80.0;
        float cell = floor(q.x / 90.0);
        float cx = (cell + 0.5) * 90.0;
        float len = 80.0 + h1(cell) * (near ? 260.0 : 180.0);
        float stal = (1.0 - abs(q.x - cx) / (24.0 + h1(cell * 2.0) * 20.0)) * len;
        if (stal > 0.0 && q.y < stal) {
            y = -1e5; // fills from the top down to the tip
            y2 = q.y < stal ? -1e5 : 1e5;
        }
    } else if (region == 3u) {
        // Willow domes with hanging strands over a water line.
        y = roundLine(q.x, near ? 380.0 : 260.0, H * (near ? 0.66 : 0.56), near ? 130.0 : 90.0, near ? 220.0 : 160.0, 11.0);
        float strands = abs(fract(q.x / 14.0) - 0.5);
        float hang = y + 40.0 + h1(floor(q.x / 14.0)) * 90.0;
        if (q.y > y && q.y < hang && strands > 0.3) {
            y2 = 1e5;
        }
    } else if (region == 4u) {
        // Columns and broken arches on sandy dunes.
        y = H * (near ? 0.78 : 0.66) - valueNoise(vec2(q.x * 0.004, 5.0)) * 90.0;
        float period = near ? 330.0 : 240.0;
        float cell = floor(q.x / period);
        float cx = (cell + 0.5) * period;
        float colH = (near ? 380.0 : 300.0) * (0.45 + 0.55 * h1(cell * 1.3));
        float colW = near ? 34.0 : 24.0;
        if (h1(cell * 2.2) > 0.3 && abs(q.x - cx) < colW) {
            y = min(y, H * (near ? 0.78 : 0.66) - colH);
        }
        if (h1(cell * 2.2) > 0.3 && abs(q.x - cx) < colW * 1.4 && abs(q.y - (H * (near ? 0.78 : 0.66) - colH)) < 10.0) {
            y = min(y, H * (near ? 0.78 : 0.66) - colH - 10.0);
        }
    } else {
        // Snowy peaks.
        y = peakLine(q.x, near ? 560.0 : 420.0, H * (near ? 0.78 : 0.7), near ? 520.0 : 460.0, near ? 3.0 : 9.0);
    }
    float top = min(y, y2);
    float a = smoothstep(top - 1.5, top + 1.5, q.y);
    if (a <= 0.0) {
        return vec4(0.0);
    }
    vec3 c = main;
    // A light band along the top edge (towards the sun) and a soft fade down.
    float rim = 1.0 - smoothstep(0.0, near ? 10.0 : 7.0, q.y - top);
    c = mix(c, second, rim * 0.55);
    if (region == 5u) {
        // Snow caps above the snow line.
        float snow = smoothstep(H * 0.5 + valueNoise(vec2(q.x * 0.02, 1.0)) * 60.0 + 4.0, H * 0.5 + valueNoise(vec2(q.x * 0.02, 1.0)) * 60.0, q.y);
        c = mix(c, second, snow);
    }
    if (region == 2u) {
        // Glowing crystals in the cave wall.
        vec3 v = voronoi(q * 0.012);
        float crystal = (1.0 - smoothstep(0.05, 0.09, v.x)) * step(0.8, fract(v.z * 9.0));
        c += vec3(0.5, 0.9, 1.2) * crystal * (0.7 + 0.3 * sin(sprite.time * 1.5 + v.z * 20.0));
    }
    if (region == 0u && !near && night > 0.05 && y2 < y) {
        // Lit windows in the distant houses.
        float cell = floor(q.x / 150.0);
        float cx = (cell + 0.5) * 150.0;
        vec2 wq = vec2(abs(q.x - cx) - 14.0, q.y - (H * 0.6 - 60.0 - h1(cell * 4.1) * 40.0 + 46.0));
        float win = 1.0 - smoothstep(4.0, 5.5, max(abs(wq.x), abs(wq.y)));
        c = mix(c, vec3(1.0, 0.8, 0.45) * 1.4, win * night);
    }
    // Atmospheric fade downwards.
    c = mix(c, main * 0.92, smoothstep(top, top + H * 0.5, q.y) * 0.4);
    return vec4(c, a * sprite.color.a);
}

vec4 light(vec2 uv, vec2 q, uint region, float night)
{
    vec2 px = uv * sprite.size;
    if (region == 0u || region == 1u) {
        // Soft sun rays from the upper left (day).
        vec2 dir = normalize(vec2(0.45, 1.0));
        float along = dot(px, vec2(dir.y, -dir.x));
        float ray = pow(max(sin(along * 0.012 + sin(along * 0.003 + sprite.time * 0.2) * 2.0), 0.0), 6.0);
        float fade = (1.0 - uv.y) * (1.0 - night);
        return vec4(sprite.color.rgb, ray * fade * 0.16 * sprite.color.a);
    }
    if (region == 3u) {
        // Drifting mist bands low down.
        float band = smoothstep(0.55, 1.0, uv.y) * (0.5 + 0.5 * fbm(vec2(q.x * 0.002 + sprite.time * 0.02, uv.y * 3.0), 3));
        return vec4(sprite.color.rgb, band * 0.45 * sprite.color.a);
    }
    if (region == 4u) {
        float glow = smoothstep(0.3, 1.0, uv.y) * 0.25;
        return vec4(sprite.color.rgb, glow * sprite.color.a);
    }
    if (region == 5u) {
        float haze = (0.5 + 0.5 * fbm(vec2(q.x * 0.003 - sprite.time * 0.15, uv.y * 4.0 + sprite.time * 0.05), 3)) * 0.25;
        return vec4(sprite.color.rgb, haze * sprite.color.a);
    }
    // Mines: a faint warm glow from below.
    return vec4(sprite.color.rgb, smoothstep(0.5, 1.0, uv.y) * 0.18 * sprite.color.a);
}

void main()
{
    vec2 uv = inUv;
    vec2 q = uv * sprite.size + sprite.params.xy;
    int layer = int(sprite.params.z + 0.5);
    float night = clamp(sprite.params.w, 0.0, 1.0);
    uint region = sprite.bits.x & 7u;
    if (layer == 0) {
        outColor = sky(uv, q, night, region);
    } else if (layer == 1) {
        outColor = clouds(q, night);
    } else if (layer == 2 || layer == 3) {
        outColor = silhouette(q, layer == 3, region, night);
    } else {
        outColor = light(uv, q, region, night);
    }
}
