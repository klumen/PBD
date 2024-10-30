#include "Particle.h"

float Particle::radius = 0.025f;
std::vector<Particle*> Particle::globalParticles;
unsigned int Particle::totPhase = 0;