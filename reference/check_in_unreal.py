"""Confronta i nodi C++ di CrowdEvents con i valori di `cases.json`.

Va eseguito dentro l'editor di Unreal, con il plugin compilato:
    UnrealEditor-Cmd.exe <progetto>.uproject -run=pythonscript -script="<percorso>/check_in_unreal.py"
oppure dalla console Python dell'editor. Scrive l'esito nel log e, se chiamato da riga
di comando, chiude con errore quando un caso non torna.
"""

import json
import os

import unreal

TOLERANCE = 1e-6
CASES_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "cases.json")

STATES = [
    unreal.CrowdReactionState.NONE,
    unreal.CrowdReactionState.AVOID,
    unreal.CrowdReactionState.LINGER,
    unreal.CrowdReactionState.FLEE,
]
ROLES = [
    unreal.CrowdReactionRole.NORMAL,
    unreal.CrowdReactionRole.AFTER_FLIGHT,
    unreal.CrowdReactionRole.SPECTATOR,
    unreal.CrowdReactionRole.RESCUER,
]


def make_field(values):
    field = unreal.CrowdEventField()
    field.set_editor_property("attractive_intensity", values[0])
    field.set_editor_property("repulsive_intensity", values[1])
    field.set_editor_property("repulsive_decay_m", values[2])
    field.set_editor_property("attractive_decay_m", values[3])
    return field


def call(function, args):
    lib = unreal.CrowdEventsLibrary
    if function in ("perceived_field", "ring_radius", "bubble_radius", "linger_reward"):
        return getattr(lib, function)(make_field(args[0]), *args[1:])
    if function == "reference_radius":
        return lib.reference_radius(STATES[args[0]], ROLES[args[1]], make_field(args[2]), *args[3:])
    if function == "reward_multipliers":
        m = lib.reward_multipliers(STATES[args[0]], ROLES[args[1]], args[2])
        return [m.get_editor_property(name) for name in ("goal", "speed", "social", "spawn", "smoothness")]
    return getattr(lib, function)(*args)


def main():
    with open(CASES_PATH, encoding="utf-8") as handle:
        cases = json.load(handle)

    failures = []
    for index, case in enumerate(cases):
        got = call(case["function"], case["args"])
        expected = case["expected"]
        got_list = got if isinstance(got, list) else [got]
        expected_list = expected if isinstance(expected, list) else [expected]
        if any(abs(g - e) > TOLERANCE for g, e in zip(got_list, expected_list)):
            failures.append((index, case["function"], case["args"], expected, got))

    version = unreal.CrowdEventsLibrary.get_crowd_events_version()
    if failures:
        for index, function, args, expected, got in failures[:20]:
            unreal.log_error(f"CrowdEvents caso {index} {function}{args}: atteso {expected}, ottenuto {got}")
        unreal.log_error(f"CrowdEvents {version}: {len(failures)} casi su {len(cases)} non tornano")
        raise RuntimeError("check_in_unreal: valori diversi dal riferimento Python")
    unreal.log(f"CrowdEvents {version}: {len(cases)} casi su {len(cases)} uguali al riferimento Python")


main()
