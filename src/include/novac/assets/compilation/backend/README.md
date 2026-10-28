# Backend de Compilation (API expérimentale, NovaC 1.4.2)

L'Engine ne dépend jamais de ce module. Compilation se construit et s'installe
uniquement sur demande (`make test-compilation`, `make install-compilation`).

**Hiérarchie d'héritage :**

- `Backend` : interface générique historique pour les backends C, bytecode, etc.
- `NativeBackend : Backend` : pipeline natif commun, preflight et publication des artefacts.
- `NativeTarget` : interface abstraite d'une architecture et plateforme native.
- `LinuxX86_64Target : NativeTarget` : fournisseur réutilisable/derivable pour x86-64 Linux.
- `X86_64Backend : NativeBackend` : façade de compatibilité pour les clients existants.

Pour une nouvelle architecture, dériver **`NativeTarget`** et enregistrer le
fournisseur dans `TargetRegistry`. Il n'est pas nécessaire de dupliquer
`NativeBackend`, d'altérer le contrôleur ou de dépendre de l'encodeur x86-64.
Pour personnaliser le fournisseur Linux x86-64, dériver
`LinuxX86_64Target` et surcharger `generate` ou `preflight` tout en conservant
les recettes et les capacités standards si elles conviennent.

`TargetSpec` précise architecture, OS, ABI, format objet, CPU et fonctionnalités.
Le registre exige actuellement une correspondance exacte. L'encodeur x86-64,
les conventions SysV et les instructions de sortie Linux restent spécifiques
au fournisseur x86-64. `NativeFramePlan` et l'écriture de conteneurs ELF64 sont
indépendants de l'encodage, sans constituer un assembleur ARM64.

Les tests de compilation sont exclusivement dans `tests/assets/compilation/`.
