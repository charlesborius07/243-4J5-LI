#ifndef TRUST_ANCHORS_H
#define TRUST_ANCHORS_H

// Ce fichier contient les certificats racines nécessaires pour une connexion 
// MQTT sécurisée (MQTTS ou WSS). 
// Si vous utilisez une connexion MQTT standard sur le port 1883 sans TLS,
// ce fichier peut rester vide ou non utilisé.

const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n" \
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n" \
"WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n" \
"ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n" \
"MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJ1yKQjQQ2n+EKX4v\n" \
"2u9t9A2t/1D8T8I8O6G/A0p8yR7D/K0H/Hh/t+H5k4x+z1b0n5x8Q/u2c/zO8aM/\n" \
"zT2aP4O0K/3V4A9mH9o1A/r9/T/uP+t+1C/t9z5m9X9e9Q9R/P9D/G/e/v1a0\n" \
"-----END CERTIFICATE-----\n";

#endif
