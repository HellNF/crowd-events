"""Formule di riferimento del plugin CrowdEvents (specifica della milestone 1).

Ogni funzione ha lo stesso nome, in snake_case, del nodo C++ di `UCrowdEventsLibrary` e
deve dare gli stessi numeri. Distanze in metri, velocità in m/s, tempi in secondi.
Solo libreria standard: gira anche con il Python dell'editor di Unreal.
"""

import math
from dataclasses import dataclass

# Stati (specifica §5.1) e ruoli, con gli stessi valori degli enum C++.
STATE_NONE, STATE_AVOID, STATE_LINGER, STATE_FLEE = 0, 1, 2, 3
ROLE_NORMAL, ROLE_AFTER_FLIGHT, ROLE_SPECTATOR, ROLE_RESCUER = 0, 1, 2, 3


@dataclass
class Field:
    """Parametri di campo di un evento (specifica §3.2)."""

    attractive_intensity: float = 1.0
    repulsive_intensity: float = 0.77
    repulsive_decay_m: float = 1.0
    attractive_decay_m: float = 4.0


def clip(value, low, high):
    return max(low, min(high, value))


def perceived_field(field, alpha, beta, distance_m):
    """Campo percepito (§4.1)."""
    value = 0.0
    if field.repulsive_decay_m > 0.0:
        value += beta * field.repulsive_intensity * math.exp(-distance_m / field.repulsive_decay_m)
    if field.attractive_decay_m > 0.0:
        value -= alpha * field.attractive_intensity * math.exp(-distance_m / field.attractive_decay_m)
    return value


def ring_radius(field, alpha, beta, contact_distance_m=0.8):
    """Distanza del minimo del campo; se non esiste, la distanza di contatto (§4.2)."""
    attraction = alpha * field.attractive_intensity
    repulsion = beta * field.repulsive_intensity
    if (
        attraction <= 0.0
        or repulsion <= 0.0
        or field.repulsive_decay_m <= 0.0
        or field.attractive_decay_m <= field.repulsive_decay_m
    ):
        return contact_distance_m
    argument = (repulsion * field.attractive_decay_m) / (attraction * field.repulsive_decay_m)
    if argument <= 1.0:
        return contact_distance_m
    return (
        field.repulsive_decay_m
        * field.attractive_decay_m
        / (field.attractive_decay_m - field.repulsive_decay_m)
        * math.log(argument)
    )


def _has_ring_shape(field):
    return field.attractive_intensity > 0.0 and field.repulsive_decay_m > 0.0 and field.attractive_decay_m > field.repulsive_decay_m


def repulsive_intensity_for_ring(field, ring_radius_m):
    """Intensità repulsiva che dà l'anello richiesto con alpha = beta = 1: l'inversa di `ring_radius`."""
    if not _has_ring_shape(field):
        return field.repulsive_intensity
    k = field.repulsive_decay_m * field.attractive_decay_m / (field.attractive_decay_m - field.repulsive_decay_m)
    return field.attractive_intensity * field.repulsive_decay_m / field.attractive_decay_m * math.exp(ring_radius_m / k)


def clamp_field_ring(field, min_ring_m=1.0, max_ring_m=5.0):
    """Il campo con l'intensità repulsiva corretta perché l'anello resti fra i due estremi."""
    if not _has_ring_shape(field):
        return Field(field.attractive_intensity, field.repulsive_intensity, field.repulsive_decay_m, field.attractive_decay_m)
    low = repulsive_intensity_for_ring(field, min_ring_m)
    high = repulsive_intensity_for_ring(field, max_ring_m)
    return Field(field.attractive_intensity, clip(field.repulsive_intensity, low, high), field.repulsive_decay_m, field.attractive_decay_m)


def sample_ring_field(rng, min_ring_m=1.0, max_ring_m=5.0, repulsion_range=(0.385, 10.0)):
    """Come `SampleRingField`, con un `random.Random`: serve a controllare le proprietà, non i numeri."""
    ring = rng.uniform(min_ring_m, max_ring_m)
    field = Field()
    for _ in range(100):
        field.attractive_intensity = rng.uniform(0.5, 2.0)
        field.repulsive_decay_m = rng.uniform(0.5, 2.0)
        field.attractive_decay_m = rng.uniform(max(2.0, field.repulsive_decay_m + 1.0), 8.0)
        field.repulsive_intensity = repulsive_intensity_for_ring(field, ring)
        if repulsion_range[0] <= field.repulsive_intensity <= repulsion_range[1]:
            break
    return field, ring


def bubble_radius(field, contact_distance_m=0.8, ring_width_m=1.5):
    """Raggio della bolla che copre il cerchio di chi guarda (§4.3)."""
    return ring_radius(field, 1.0, 1.0, contact_distance_m) + ring_width_m


def reference_radius(state, role, field, alpha, beta, safe_radius_m, contact_distance_m=0.8, ring_width_m=1.5):
    """Distanza di riferimento per stato e ruolo (§4.3); -1 se lo stato non ne ha una."""
    if state == STATE_AVOID:
        if role == ROLE_AFTER_FLIGHT:
            return safe_radius_m
        return bubble_radius(field, contact_distance_m, ring_width_m)
    if state == STATE_LINGER:
        if role == ROLE_RESCUER:
            return contact_distance_m
        return ring_radius(field, alpha, beta, contact_distance_m)
    return -1.0


def avoid_reward(beta, distance_m, bubble_radius_m, repulsive_decay_m):
    """Stato 1, bolla (§8.3): circa -1 sul bordo, fino a -3 dentro."""
    if repulsive_decay_m <= 0.0:
        return 0.0
    return -beta * min(3.0, math.exp(-(distance_m - bubble_radius_m) / repulsive_decay_m))


def safe_wall_reward(distance_m, safe_radius_m):
    """Stato 1 dopo la fuga (§8.3), senza il peso k_safe."""
    if safe_radius_m <= 0.0:
        return 0.0
    return -max(0.0, (safe_radius_m - distance_m) / safe_radius_m)


def linger_reward(field, alpha, beta, distance_m, target_radius_m, perception_radius_m, contact_distance_m=0.8):
    """Stato 2, anello o contatto (§8.3): 0 sul bersaglio, fino a -1 allontanandosi."""
    clamped = max(distance_m, contact_distance_m)
    at_target = perceived_field(field, alpha, beta, target_radius_m)
    denominator = perceived_field(field, alpha, beta, perception_radius_m) - at_target
    if denominator <= 1e-9:
        return 0.0
    return clip(-(perceived_field(field, alpha, beta, clamped) - at_target) / denominator, -1.0, 0.0)


def linger_speed_target(base_speed_ms, distance_m, target_radius_m, length_m=1.0):
    """Stato 2, velocità bersaglio (§5.5): scende a 0 arrivando sul bersaglio."""
    if length_m <= 0.0:
        return 0.0
    return base_speed_ms * math.tanh(max(0.0, distance_m - target_radius_m) / length_m)


def linger_speed_reward(speed_ms, target_speed_ms, base_speed_ms):
    """Stato 2, termine di velocità (§8.3): divide per la velocità desiderata iniziale."""
    if base_speed_ms <= 0.0:
        return 0.0
    return -clip(((speed_ms - target_speed_ms) / base_speed_ms) ** 2, 0.0, 1.0)


def flee_reward(distance_m, previous_distance_m, panic_speed_ms, delta_time_s):
    """Stato 3, allontanarsi (§8.3)."""
    if panic_speed_ms <= 0.0 or delta_time_s <= 0.0:
        return 0.0
    return clip((distance_m - previous_distance_m) / (panic_speed_ms * delta_time_s), -1.0, 1.0)


def notice_probability(
    attention, salience, distance_m, in_front, fleeing_seen, delta_time_s, perception_decay_m=10.0, contagion_weight=0.5
):
    """Probabilità di accorgersi dell'evento in un passo (§5.2)."""
    if perception_decay_m <= 0.0:
        return 0.0
    view = 1.0 if in_front else 0.3
    rate = attention * salience * math.exp(-distance_m / perception_decay_m) * view + contagion_weight * fleeing_seen
    return 1.0 - math.exp(-rate * delta_time_s)


def reward_multipliers(state, role, panic_k=0.5):
    """Moltiplicatori dei cinque termini di DCR (§8.4): goal, velocità, sociale, spawn, regolarità.

    Nello stato 2 il termine di velocità di DCR vale 0 perché lo sostituisce
    `linger_speed_reward`, che entra nella reward aggiuntiva.
    """
    if state == STATE_LINGER:
        return (0.0, 0.0, 1.0, 0.0, 1.0)
    if state == STATE_FLEE:
        return (0.0, 1.0, 1.0 - panic_k, 0.0, 1.0 - panic_k)
    return (1.0, 1.0, 1.0, 1.0, 1.0)


if __name__ == "__main__":
    # Valori di controllo del README.
    assert abs(ring_radius(Field(1.0, 0.77), 1.0, 1.0) - 1.4999) < 1e-3
    assert abs(ring_radius(Field(1.0, 5.0), 1.0, 1.0) - 3.9943) < 1e-3
    assert ring_radius(Field(0.0, 1.0, 3.0), 1.0, 1.0) == 0.8
    # Il tetto: con repulsione 10 e decadimenti 6 e 8 m l'anello sarebbe di circa 62 m.
    wide = Field(1.0, 10.0, 6.0, 8.0)
    assert ring_radius(wide, 1.0, 1.0) > 60.0
    assert abs(ring_radius(clamp_field_ring(wide), 1.0, 1.0) - 5.0) < 1e-9
    import random

    rng = random.Random(1)
    for _ in range(10000):
        sampled, ring = sample_ring_field(rng)
        assert 1.0 <= ring <= 5.0 and abs(ring_radius(sampled, 1.0, 1.0) - ring) < 1e-9
        assert 0.385 <= sampled.repulsive_intensity <= 10.0
    print("ok")
