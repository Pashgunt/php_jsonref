# 🧩 PHP Extension JSON REF
![PHP version](https://img.shields.io/badge/php-%3E=8.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen)

> **JsonRef** - is a PHP extension written in C that allows you to read and modify JSON strings directly without converting them to PHP arrays or objects.
> This is achieved by parsing the path and searching for the desired node in the string, and then safely changing the values on the fly.

---

## 📌 It is important to understand

### ✅ Where it helps:

- High performance, no array/object is created in PHP memory, json_decode() is not used
- Lower memory consumption, suitable for large JSON (for example, >100 MB) that do not fit into memory after json_decode()
- Direct editing, changes are applied directly to the string, without serialization/deserialization
- Dot notation support,Full access to nested fields: a.b.c.d
- Auto-creation of paths, with json_set, nested objects are created automatically if they do not exist
- Security, no creation of PHP objects from JSON — fewer vulnerabilities (for example, when working with untrusted data)

---

## 🚀 Install

### 📦 Source code build

```bash
git clone https://github.com/Pashgunt/php_jsonref.git
cd php_jsonref
mkdir build && cd build
cmake ..
make
sudo make install
```

Add to `php.ini`:
```ini
extension=jsonref.so
```

### 🐘 (Future) Install from PECL

```bash
pecl install jsonref
```

---

## ✏️ Examples and result primitive benchmarks

```php
$bigData = [
    'users' => []
];

for ($i = 0; $i < 50000; $i++) {
    $bigData['users'][] = [
        'id' => $i,
        'name' => "User$i",
        'age' => rand(18, 80),
        'email' => "user$i@example.com",
        'tags' => ['tag1', 'tag2', 'tag3'],
    ];
}

$bigJson = json_encode($bigData, JSON_UNESCAPED_UNICODE);
$iterations = 10;

function benchmark_jsonref($json, $iterations) {
    $timeStart = microtime(true);

    for ($i = 0; $i < $iterations; $i++) {
        $age = json_get($json, 'users.1000.age');
        $json = json_set($json, 'users.1000.age', $age + 1);
    }

    return microtime(true) - $timeStart;
}

function benchmark_json_decode_encode($json, $iterations) {
    $timeStart = microtime(true);

    for ($i = 0; $i < $iterations; $i++) {
        $data = json_decode($json, true);
        $data['users'][1000]['age'] += 1;
        $json = json_encode($data, JSON_UNESCAPED_UNICODE);
    }

    return microtime(true) - $timeStart;
}

echo "Starting benchmarks...\n";

$timeJsonref = benchmark_jsonref($bigJson, $iterations);
$timeDecodeEncode = benchmark_json_decode_encode($bigJson, $iterations);

echo "jsonref time for $iterations iterations: {$timeJsonref} sec\n";
echo "json_decode+json_encode time for $iterations iterations: {$timeDecodeEncode} sec\n";

if ($timeJsonref < $timeDecodeEncode) {
    $percentFaster = (($timeDecodeEncode - $timeJsonref) / $timeDecodeEncode) * 100;
    echo "jsonref is faster by approximately " . round($percentFaster, 2) . "%\n";
} else {
    $percentSlower = (($timeJsonref - $timeDecodeEncode) / $timeJsonref) * 100;
    echo "jsonref is slower by approximately " . round($percentSlower, 2) . "%\n";
}
```

## Average result:

```
jsonref time for 10 iterations: 0.23355007171631 sec
json_decode+json_encode time for 10 iterations: 0.55796909332275 sec
jsonref is faster by approximately 58.14%
```
