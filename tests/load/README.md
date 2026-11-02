# Manual load / DoS trials

These scripts are manual performance and resilience trials, not part of the
automated test suite. They assume a server is already running on
`127.0.0.1:8080` (see `run_ammarserver` in the repository root) and require
external tools that may not be installed:

- `siege_trial.sh` – one `siege` run with a configurable number of concurrent
  connections (default 1300) for one minute.
- `multiSiege.sh` – several concurrent `siege` instances (default 10), each
  with 1000 connections, and a summary of transactions/failures/rate.
- `dos_trial.sh` – a `wget`-based concurrent download trial that also compares
  the received files for corruption and plots response times with `gnuplot`.

Install the tools first, for example on Debian/Ubuntu:

```bash
sudo apt-get install siege wget gnuplot
```
