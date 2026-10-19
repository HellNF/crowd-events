"""Scrive `cases.json`: ingressi e valori attesi per ogni formula del plugin.

Uso: python reference/generate_cases.py
Il file va rigenerato e committato a ogni modifica di `crowd_events_reference.py`.
"""

import itertools
import json
from pathlib import Path

import crowd_events_reference as ref

FIELDS = {
    "street_performer": ref.Field(1.0, 0.77, 1.0, 4.0),
    "brawl": ref.Field(1.0, 5.0, 1.0, 4.0),
    "fallen_person": ref.Field(1.0, 0.77, 1.0, 4.0),
    "danger": ref.Field(0.0, 1.0, 3.0, 4.0),
}
PERCEPTION_RADIUS = {"street_performer": 10.0, "brawl": 10.0, "fallen_person": 8.0, "danger": 15.0}
DISTANCES = [0.0, 0.5, 0.8, 1.5, 3.0, 4.0, 6.0, 10.0, 15.0, 20.0]
SENSITIVITIES = [(1.0, 1.0), (0.5, 1.0), (1.0, 0.0), (2.0, 0.5)]


def field_args(field):
    return [field.attractive_intensity, field.repulsive_intensity, field.repulsive_decay_m, field.attractive_decay_m]


def build_cases():
    cases = []

    def add(function, args, expected):
        cases.append({"function": function, "args": args, "expected": expected})

    for name, field in FIELDS.items():
        add("bubble_radius", [field_args(field), 0.8, 1.5], ref.bubble_radius(field))
        for alpha, beta in SENSITIVITIES:
            add("ring_radius", [field_args(field), alpha, beta, 0.8], ref.ring_radius(field, alpha, beta))
            for state, role in itertools.product(range(4), range(4)):
                add(
                    "reference_radius",
                    [state, role, field_args(field), alpha, beta, 15.0, 0.8, 1.5],
                    ref.reference_radius(state, role, field, alpha, beta, 15.0),
                )
            for distance in DISTANCES:
                add("perceived_field", [field_args(field), alpha, beta, distance], ref.perceived_field(field, alpha, beta, distance))
                for target in (ref.ring_radius(field, alpha, beta), 0.8):
                    add(
                        "linger_reward",
                        [field_args(field), alpha, beta, distance, target, PERCEPTION_RADIUS[name], 0.8],
                        ref.linger_reward(field, alpha, beta, distance, target, PERCEPTION_RADIUS[name]),
                    )
        for distance in DISTANCES:
            for beta in (0.5, 1.0):
                bubble = ref.bubble_radius(field)
                add(
                    "avoid_reward",
                    [beta, distance, bubble, field.repulsive_decay_m],
                    ref.avoid_reward(beta, distance, bubble, field.repulsive_decay_m),
                )

    for distance in DISTANCES:
        add("safe_wall_reward", [distance, 15.0], ref.safe_wall_reward(distance, 15.0))
        for base_speed, target in itertools.product((1.0, 1.5, 2.0), (0.8, 1.5, 4.0)):
            add("linger_speed_target", [base_speed, distance, target, 1.0], ref.linger_speed_target(base_speed, distance, target))
        for attention, salience, in_front, fleeing in itertools.product((1.0, 0.2), (0.5, 1.0, 2.0), (True, False), (0, 2)):
            add(
                "notice_probability",
                [attention, salience, distance, in_front, fleeing, 0.1, 10.0, 0.5],
                ref.notice_probability(attention, salience, distance, in_front, fleeing, 0.1),
            )

    for speed, target, base in itertools.product((0.0, 0.2, 1.0, 2.5), (0.0, 0.5, 1.5), (1.0, 2.0)):
        add("linger_speed_reward", [speed, target, base], ref.linger_speed_reward(speed, target, base))

    ring_fields = list(FIELDS.values()) + [
        ref.Field(1.0, 10.0, 6.0, 8.0),
        ref.Field(1.0, 0.1, 1.0, 4.0),
        ref.Field(2.0, 3.0, 0.5, 8.0),
        ref.Field(0.5, 1.0, 2.0, 3.0),
    ]
    for field in ring_fields:
        for ring in (1.0, 1.5, 3.0, 5.0):
            add("repulsive_intensity_for_ring", [field_args(field), ring], ref.repulsive_intensity_for_ring(field, ring))
        clamped = ref.clamp_field_ring(field, 1.0, 5.0)
        add("clamp_field_ring", [field_args(field), 1.0, 5.0], field_args(clamped))

    for step, panic in itertools.product((-0.4, -0.1, 0.0, 0.1, 0.275, 0.5), (2.5, 3.0)):
        add("flee_reward", [8.0 + step, 8.0, panic, 0.1], ref.flee_reward(8.0 + step, 8.0, panic, 0.1))

    for state, role in itertools.product(range(4), range(4)):
        add("reward_multipliers", [state, role, 0.5], list(ref.reward_multipliers(state, role)))

    return cases


if __name__ == "__main__":
    cases = build_cases()
    output = Path(__file__).with_name("cases.json")
    output.write_text(json.dumps(cases, indent=1) + "\n", encoding="utf-8")
    print(f"{len(cases)} casi scritti in {output.name}")
