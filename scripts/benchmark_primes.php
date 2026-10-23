<?php
// benchmark_primes.php
//
// Synthetic non-trivial-compute benchmark counterpart to prepare_primes_content_callback() in
// src/Services/AmmarServer/main.c - counts primes below N via a Sieve of Eratosthenes and sums them, so this
// does real, deterministic CPU work per request rather than just echoing something back. The prime count and
// sum are printed specifically so the two implementations' outputs can be diffed for correctness ( they should
// match exactly for the same N ).
//
// Deploy this under a PHP-capable web server ( e.g. copy it into an Apache docroot with mod_php enabled ) and
// benchmark it the same way as the C version, e.g. with wrk:
//   wrk -t4 -c100 -d8s http://127.0.0.1/benchmark_primes.php

$N = 100000;

$start = microtime(true);

$sieve = array_fill(0, $N + 1, true);
$sieve[0] = false;
$sieve[1] = false;

for ($i = 2; $i * $i <= $N; $i++) {
    if ($sieve[$i]) {
        for ($j = $i * $i; $j <= $N; $j += $i) {
            $sieve[$j] = false;
        }
    }
}

$count = 0;
$sum = 0;
for ($i = 2; $i <= $N; $i++) {
    if ($sieve[$i]) {
        $count++;
        $sum += $i;
    }
}

$elapsed_ms = (microtime(true) - $start) * 1000;

echo "<html><body>N=$N primes_below_n=$count sum_of_primes=$sum elapsed_ms=" . round($elapsed_ms, 3) . "</body></html>";
