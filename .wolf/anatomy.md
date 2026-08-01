# anatomy.md

> Auto-maintained by OpenWolf. Last scanned: 2026-08-01T15:46:13.681Z
> Files: 274 tracked | Anatomy hits: 0 | Misses: 0

## ./

- `.gitignore` — Git ignore rules (~71 tok)
- `CLAUDE.md` — CLAUDE.md (~1219 tok)

## .claude/

- `settings.json` (~536 tok)
- `settings.local.json` (~115 tok)

## .claude/commands/

- `reframe.md` — Mode: migrate [framework] (~551 tok)
- `security-audit.md` — Layer 1 — Dependencies (~510 tok)

## .claude/rules/

- `openwolf.md` (~328 tok)

## .claude/skills/bmad-advanced-elicitation/

- `methods.csv` (~3860 tok)
- `SKILL.md` — Advanced Elicitation (~1588 tok)

## .claude/skills/bmad-agent-analyst/

- `customize.toml` — /project-context.md", (~957 tok)
- `SKILL.md` — Mary — Business Analyst (~1127 tok)

## .claude/skills/bmad-agent-architect/

- `customize.toml` — /project-context.md", (~776 tok)
- `SKILL.md` — Winston — System Architect (~1120 tok)

## .claude/skills/bmad-agent-dev/

- `customize.toml` — /project-context.md", (~952 tok)
- `SKILL.md` — Amelia — Senior Software Engineer (~1136 tok)

## .claude/skills/bmad-agent-pm/

- `customize.toml` — /project-context.md", (~867 tok)
- `SKILL.md` — John — Product Manager (~1120 tok)

## .claude/skills/bmad-agent-tech-writer/

- `customize.toml` — /project-context.md", (~945 tok)
- `explain-concept.md` — Explain Concept (~170 tok)
- `mermaid-gen.md` — Mermaid Generate (~169 tok)
- `SKILL.md` — Paige — Technical Writer (~1141 tok)
- `validate-doc.md` — Validate Documentation (~164 tok)
- `write-document.md` — Write Document (~231 tok)

## .claude/skills/bmad-agent-ux-designer/

- `customize.toml` — /project-context.md", (~723 tok)
- `SKILL.md` — Sally — UX Designer (~1125 tok)

## .claude/skills/bmad-architecture/

- `customize.toml` — /project-context.md", (~1735 tok)
- `SKILL.md` — BMad Architecture (~3446 tok)

## .claude/skills/bmad-architecture/assets/

- `spine-template.md` — Architecture Spine — {name} (~1083 tok)

## .claude/skills/bmad-architecture/references/

- `headless.md` — Headless (~662 tok)
- `reviewer-gate.md` — Reviewer Gate (~807 tok)

## .claude/skills/bmad-architecture/scripts/

- `lint_spine.py` — lint-spine — the mechanical half of spine decision-integrity, done deterministically. (~2989 tok)
  - fn `split_frontmatter` L42-59 (~226 tok)
  - fn `blank_fences` L60-65 (~76 tok)
  - fn `line_of` L66-69 (~24 tok)
  - fn `find_placeholders` L70-90 (~274 tok)
  - fn `find_frontmatter_placeholders` L91-108 (~226 tok)
  - fn `find_ad_issues` L109-152 (~485 tok)
  - fn `find_unpinned_stack` L153-200 (~617 tok)
  - fn `_table_cells` L201-210 (~82 tok)
  - fn `lint` L211-229 (~177 tok)
  - fn `main` L230-258 (~331 tok)

## .claude/skills/bmad-architecture/scripts/tests/

- `test_lint_spine.py` — Tests for lint_spine.py. Run: uv run --with pytest pytest scripts/tests/test_lint_spine.py (~2830 tok)
  - fn `cats` L59-62 (~23 tok)
  - fn `test_clean_spine_passes` L63-68 (~41 tok)
  - fn `test_mermaid_braces_not_flagged` L69-74 (~60 tok)
  - fn `test_placeholder_markers_caught` L75-80 (~48 tok)
  - fn `test_similar_to_caught` L81-86 (~59 tok)
  - fn `test_unfilled_template_token_caught` L87-92 (~62 tok)
  - fn `test_duplicate_ad_id_caught` L93-98 (~57 tok)
  - fn `test_non_monotonic_ad_id_caught` L99-106 (~94 tok)
  - fn `test_missing_field_caught` L107-112 (~74 tok)
  - fn `test_unpinned_dep_caught` L113-118 (~50 tok)
  - fn `test_placeholder_version_caught` L119-124 (~72 tok)
  - fn `test_no_stack_section_ok` L125-130 (~44 tok)
  - fn `test_stack_skeleton_row_not_version_pinned` L131-137 (~93 tok)
  - fn `test_stack_html_comment_not_parsed_as_row` L138-143 (~65 tok)
  - fn `test_template_token_is_low_severity` L144-152 (~132 tok)
  - fn `test_no_frontmatter_body_still_scanned` L153-158 (~77 tok)
  - fn `test_frontmatter_value_with_dashes_not_truncated` L159-166 (~123 tok)
  - fn `test_ad_heading_in_fence_not_counted` L167-176 (~114 tok)
  - fn `test_stack_table_flags_only_the_unpinned_row` L177-184 (~103 tok)
  - fn `test_stack_table_all_pinned_ok` L185-191 (~69 tok)
  - fn `test_fenced_stack_rows_not_parsed` L192-200 (~134 tok)
  - fn `test_fenced_stack_heading_not_live` L201-207 (~84 tok)
  - fn `test_renamed_stack_heading_still_scanned` L208-216 (~124 tok)
  - fn `test_reordered_columns_pair_name_to_version` L217-225 (~128 tok)
  - fn `test_placeholder_line_number_is_absolute` L226-239 (~132 tok)
  - fn `test_missing_spine_file_reports_error` L240-245 (~69 tok)
  - fn `test_frontmatter_unfilled_token_caught` L246-253 (~127 tok)
  - fn `test_frontmatter_tbd_caught` L254-260 (~82 tok)
  - fn `test_unreadable_spine_returns_error_not_crash` L261-271 (~144 tok)

## .claude/skills/bmad-brainstorming/

- `customize.toml` — /project-context.md", (~1229 tok)
- `SKILL.md` — BMad Brainstorming (~2336 tok)

## .claude/skills/bmad-brainstorming/analysis/

- `catalog-analysis.md` — BMad Brainstorming Catalog — Deep Analysis (~5825 tok)
- `method-matrix.csv` (~2355 tok)

## .claude/skills/bmad-brainstorming/assets/

- `brain-icons.json` (~11781 tok)
- `brain-methods.csv` — Declares and (~5187 tok)
- `brain-selector.html` — BMad Method Brainstorming Selection (~65087 tok)

## .claude/skills/bmad-brainstorming/references/

- `converge.md` — Converging: Narrow & Decide (~733 tok)
- `finalize.md` — Wrap-Up: Synthesis & Artifacts (~1219 tok)
- `headless.md` — Headless Mode (~1241 tok)
- `in-chat-techniques.md` — Choosing Techniques In Chat (~543 tok)
- `mode-autonomous.md` — Mode: Ideate For Me (~442 tok)
- `mode-facilitator.md` — Mode: Facilitator (~241 tok)
- `mode-partner.md` — Mode: Creative Partner (~399 tok)
- `resume.md` — Resuming a Session (~183 tok)

## .claude/skills/bmad-brainstorming/scripts/

- `brain.py` — Serve the brainstorming technique library without loading it all into context. (~10392 tok)
  - fn `load` L50-59 (~80 tok)
  - fn `load_extra` L60-80 (~288 tok)
  - fn `categories` L81-87 (~60 tok)
  - fn `filter_cats` L88-94 (~62 tok)
  - fn `find` L95-103 (~90 tok)
  - fn `resolve_detail` L104-115 (~150 tok)
  - fn `fmt_categories` L116-121 (~61 tok)
  - fn `fmt_list` L122-127 (~80 tok)
  - fn `fmt_show` L128-147 (~181 tok)
  - fn `pretty` L148-176 (~415 tok)
  - fn `_load_icons` L177-189 (~140 tok)
  - fn `_hsl_hex` L190-196 (~61 tok)
  - fn `category_style` L197-205 (~120 tok)
  - fn `tech_icon` L206-547 (~5285 tok)
  - fn `_good_for_label` L548-552 (~49 tok)
  - fn `_svg` L553-556 (~39 tok)
  - fn `_card` L557-577 (~320 tok)
  - fn `_invent_card` L578-587 (~142 tok)
  - fn `html_doc` L588-668 (~1040 tok)
  - fn `main` L669-741 (~1044 tok)

## .claude/skills/bmad-brainstorming/scripts/tests/

- `test_brain.py` — Tests for brain.py. Run: uv run -m pytest scripts/tests/test_brain.py (~2351 tok)
  - fn `lib` L25-32 (~73 tok)
  - fn `test_load_normalizes_detail` L33-39 (~53 tok)
  - fn `test_categories_counts_sorted` L40-43 (~42 tok)
  - fn `test_filter_is_case_insensitive` L44-48 (~55 tok)
  - fn `test_filter_none_returns_all` L49-52 (~30 tok)
  - fn `test_find_hits_and_misses` L53-58 (~62 tok)
  - fn `test_resolve_detail_present` L59-63 (~50 tok)
  - fn `test_resolve_detail_absent_is_none` L64-68 (~48 tok)
  - fn `test_resolve_detail_missing_file_warns_not_fatal` L69-75 (~72 tok)
  - fn `test_show_inlines_detail` L76-81 (~64 tok)
  - fn `test_show_simple_has_no_detail` L82-87 (~54 tok)
  - fn `test_show_all_missing_returns_1` L88-91 (~32 tok)
  - fn `test_list_filtered_text` L92-97 (~72 tok)
  - fn `test_list_bare_is_refused` L98-105 (~96 tok)
  - fn `test_list_all_dumps_everything` L106-111 (~69 tok)
  - fn `test_json_output` L112-118 (~61 tok)
  - fn `test_random_respects_n_and_category` L119-125 (~89 tok)
  - fn `test_random_negative_n_does_not_crash` L126-131 (~75 tok)
  - fn `test_missing_file_returns_2` L132-137 (~58 tok)
  - fn `test_html_requires_out` L138-143 (~64 tok)
  - fn `test_html_writes_selection_page` L144-154 (~142 tok)
  - fn `test_html_creates_missing_parent` L155-170 (~170 tok)
  - fn `extra` L171-176 (~32 tok)
  - fn `test_extra_merges_into_categories` L177-183 (~94 tok)
  - fn `test_extra_appears_in_list_and_random` L184-188 (~65 tok)
  - fn `test_extra_is_first_class_in_html` L189-197 (~119 tok)
  - fn `test_extra_missing_file_returns_2` L198-201 (~46 tok)
  - fn `test_unknown_category_style_uses_fallback_glyph` L202-207 (~67 tok)
  - fn `test_shipped_selector_is_in_sync_with_catalog` L208-218 (~174 tok)

## .claude/skills/bmad-check-implementation-readiness/

- `customize.toml` — /project-context.md", (~465 tok)
- `SKILL.md` — Implementation Readiness (~1216 tok)

## .claude/skills/bmad-check-implementation-readiness/steps/

- `step-01-document-discovery.md` — Step 1: Document Discovery (~1223 tok)
- `step-02-prd-analysis.md` — Step 2: PRD Analysis (~1101 tok)
- `step-03-epic-coverage-validation.md` — Step 3: Epic Coverage Validation (~1003 tok)
- `step-04-ux-alignment.md` — Step 4: UX Alignment (~816 tok)
- `step-05-epic-quality-review.md` — Step 5: Epic Quality Review (~1667 tok)
- `step-06-final-assessment.md` — Step 6: Final Assessment (~878 tok)

## .claude/skills/bmad-check-implementation-readiness/templates/

- `readiness-report-template.md` — Implementation Readiness Assessment Report (~24 tok)

## .claude/skills/bmad-checkpoint-preview/

- `customize.toml` — /project-context.md", (~461 tok)
- `generate-trail.md` — Generate Review Trail (~493 tok)
- `SKILL.md` — Checkpoint Review Workflow (~786 tok)
- `step-01-orientation.md` — Step 1: Orientation (~1238 tok)
- `step-02-walkthrough.md` — Step 2: Walkthrough (~1029 tok)
- `step-03-detail-pass.md` — Step 3: Detail Pass (~1231 tok)
- `step-04-testing.md` — Step 4: Testing (~665 tok)
- `step-05-wrapup.md` — Step 5: Wrap-Up (~376 tok)

## .claude/skills/bmad-code-review/

- `customize.toml` — /project-context.md", (~460 tok)
- `SKILL.md` — Code Review Workflow (~1080 tok)

## .claude/skills/bmad-code-review/steps/

- `step-01-gather-context.md` — Step 1: Gather Context (~1612 tok)
- `step-02-review.md` — Step 2: Review (~516 tok)
- `step-03-triage.md` — Step 3: Triage (~843 tok)
- `step-04-present.md` — Step 4: Present and Act (~1573 tok)

## .claude/skills/bmad-correct-course/

- `checklist.md` — Change Navigation Checklist (~2909 tok)
- `customize.toml` — /project-context.md", (~468 tok)
- `SKILL.md` — Correct Course - Sprint Change Management Workflow (~3381 tok)

## .claude/skills/bmad-create-architecture/

- `customize.toml` — /project-context.md", (~477 tok)
- `SKILL.md` — DEPRECATED — forwards to bmad-architecture (create intent) (~760 tok)

## .claude/skills/bmad-create-epics-and-stories/

- `customize.toml` — /project-context.md", (~474 tok)
- `SKILL.md` — Create Epics and Stories (~1265 tok)

## .claude/skills/bmad-create-epics-and-stories/steps/

- `step-01-validate-prerequisites.md` — Step 1: Validate Prerequisites and Extract Requirements (~2727 tok)
- `step-02-design-epics.md` — Step 2: Design Epic List (~2188 tok)
- `step-03-create-stories.md` — Step 3: Generate Epics and Stories (~2072 tok)
- `step-04-final-validation.md` — Step 4: Final Validation (~1318 tok)

## .claude/skills/bmad-create-epics-and-stories/templates/

- `epics-template.md` — {{project_name}} - Epic Breakdown (~277 tok)

## .claude/skills/bmad-create-prd/

- `customize.toml` — /project-context.md", (~461 tok)
- `SKILL.md` — DEPRECATED — forwards to bmad-prd (create intent) (~717 tok)

## .claude/skills/bmad-create-story/

- `checklist.md` — 🎯 Story Context Quality Competition Prompt (~3574 tok)
- `customize.toml` — /project-context.md", (~469 tok)
- `discover-inputs.md` — Discover Inputs Protocol (~978 tok)
- `SKILL.md` — Create Story Workflow (~5866 tok)
- `template.md` — Story {{epic_num}}.{{story_num}}: {{story_title}} (~237 tok)

## .claude/skills/bmad-customize/

- `SKILL.md` — BMad Customize (~1661 tok)

## .claude/skills/bmad-customize/scripts/

- `list_customizable_skills.py` — Enumerate customizable BMad skills installed alongside this one. (~2301 tok)
  - fn `default_skills_root` L44-52 (~89 tok)
  - fn `read_frontmatter_description` L53-81 (~295 tok)
  - fn `load_customize` L82-90 (~75 tok)
  - fn `scan_skills` L91-167 (~808 tok)
  - fn `parse_args` L168-202 (~327 tok)
  - fn `main` L203-232 (~251 tok)

## .claude/skills/bmad-customize/scripts/tests/

- `test_list_customizable_skills.py` — Unit tests for list_customizable_skills.py. (~2641 tok)
  - fn `_load_module` L32-41 (~76 tok)
  - fn `_make_skill` L42-50 (~104 tok)
  - class `ScannerTest` L51-250 (~2229 tok)

## .claude/skills/bmad-dev-auto/

- `compile-epic-context.md` — Compile Epic Context (~747 tok)
- `customize.toml` — /project-context.md", (~250 tok)
- `SKILL.md` — Dev Auto Workflow (~1221 tok)
- `spec-template.md` — Intent (~1015 tok)
- `step-01-clarify-and-route.md` — Step 1: Clarify and Route (~1550 tok)
- `step-02-plan.md` — Step 2: Plan (~541 tok)
- `step-03-implement.md` — Step 3: Implement (~580 tok)
- `step-04-review.md` — Step 4: Review (~1677 tok)

## .claude/skills/bmad-dev-story/

- `checklist.md` — 🎯 Enhanced Definition of Done Checklist (~1092 tok)
- `customize.toml` — /project-context.md", (~460 tok)
- `SKILL.md` — Dev Story Workflow (~6661 tok)

## .claude/skills/bmad-document-project/

- `checklist.md` — Document Project Workflow - Validation Checklist (~2520 tok)
- `customize.toml` — /project-context.md", (~455 tok)
- `documentation-requirements.csv` — ;**/*.test.*;**/*.spec.*,.env*;config/*;*.config.*;.config/;settings/,*auth*.ts;*session*.ts;middleware/auth*;*.guard.ts;*authenticat*;*permission*... (~2154 tok)
- `instructions.md` — Document Project Workflow Router (~1291 tok)
- `SKILL.md` — Document Project Workflow (~665 tok)

## .claude/skills/bmad-document-project/templates/

- `deep-dive-template.md` — {{target_name}} - Deep Dive Documentation (~1462 tok)
- `index-template.md` — {{project_name}} Documentation Index (~1124 tok)
- `project-overview-template.md` — {{project_name}} - Project Overview (~488 tok)
- `project-scan-report-schema.json` (~1382 tok)
- `source-tree-template.md` — {{project_name}} - Source Tree Analysis (~540 tok)

## .claude/skills/bmad-document-project/workflows/

- `deep-dive-instructions.md` — Deep-Dive Documentation Instructions (~2994 tok)
- `deep-dive-workflow.md` — Deep-Dive Documentation Sub-Workflow (~244 tok)
- `full-scan-instructions.md` — Full Project Scan Instructions (~10741 tok)
- `full-scan-workflow.md` — Full Project Scan Sub-Workflow (~242 tok)

## .claude/skills/bmad-domain-research/

- `customize.toml` — /project-context.md", (~476 tok)
- `research.template.md` — Research Report: {{research_type}} (~146 tok)
- `SKILL.md` — Domain Research Workflow (~1189 tok)

## .claude/skills/bmad-domain-research/domain-steps/

- `step-01-init.md` — Domain Research Step 1: Domain Research Scope Confirmation (~1332 tok)
- `step-02-domain-analysis.md` — Domain Research Step 2: Industry Analysis (~2143 tok)
- `step-03-competitive-landscape.md` — Domain Research Step 3: Competitive Landscape (~2364 tok)
- `step-04-regulatory-focus.md` — Domain Research Step 4: Regulatory Focus (~1726 tok)
- `step-05-technical-trends.md` — Domain Research Step 5: Technical Trends (~1859 tok)
- `step-06-research-synthesis.md` — Domain Research Step 6: Research Synthesis and Completion (~4287 tok)

## .claude/skills/bmad-edit-prd/

- `customize.toml` — /project-context.md", (~485 tok)
- `SKILL.md` — DEPRECATED — forwards to bmad-prd (update intent) (~726 tok)

## .claude/skills/bmad-editorial-review-prose/

- `SKILL.md` — Editorial Review - Prose (~1271 tok)

## .claude/skills/bmad-editorial-review-structure/

- `SKILL.md` — Editorial Review - Structure (~2705 tok)

## .claude/skills/bmad-forge-idea/

- `customize.toml` — /project-context.md", (~503 tok)
- `SKILL.md` — BMad Forge Idea (~2527 tok)

## .claude/skills/bmad-forge-idea/scripts/

- `resolve_personas.py` — Resolve the personas and parties the forge can bring into the room. (~2830 tok)
  - fn `_run_json` L48-61 (~124 tok)
  - fn `_load_toml` L62-72 (~78 tok)
  - fn `load_agents` L73-90 (~212 tok)
  - fn `find_party_skill` L91-107 (~161 tok)
  - fn `load_party_workflow` L108-118 (~175 tok)
  - fn `load_party_overrides` L119-138 (~235 tok)
  - fn `_alias` L139-146 (~71 tok)
  - fn `build_pool` L147-200 (~592 tok)
  - fn `_brief` L201-209 (~90 tok)
  - fn `resolve_parties` L210-230 (~197 tok)
  - fn `main` L231-261 (~332 tok)
  - fn `_emit` L262-271 (~75 tok)

## .claude/skills/bmad-forge-idea/scripts/tests/

- `test_resolve_personas.py` — Unit tests for resolve_personas.py — pool merge, alias, party resolution. (~1810 tok)
  - class `TestAlias` L20-28 (~89 tok)
  - class `TestBuildPool` L29-76 (~722 tok)
  - class `TestResolveParties` L77-117 (~581 tok)
  - class `TestOverrideMergeFallback` L118-139 (~282 tok)

## .claude/skills/bmad-generate-project-context/

- `customize.toml` — /project-context.md", (~469 tok)
- `project-context-template.md` — Project Context for AI Agents (~136 tok)
- `SKILL.md` — Generate Project Context Workflow (~956 tok)

## .claude/skills/bmad-generate-project-context/steps/

- `step-01-discover.md` — Step 1: Context Discovery & Initialization (~1492 tok)
- `step-02-generate.md` — Step 2: Context Rules Generation (~2269 tok)
- `step-03-complete.md` — Step 3: Context Completion & Finalization (~2072 tok)

## .claude/skills/bmad-help/

- `SKILL.md` — BMad Help (~1146 tok)

## .claude/skills/bmad-index-docs/

- `SKILL.md` — Index Docs (~411 tok)

## .claude/skills/bmad-market-research/

- `customize.toml` — /project-context.md", (~476 tok)
- `research.template.md` — Research Report: {{research_type}} (~146 tok)
- `SKILL.md` — Market Research Workflow (~1174 tok)

## .claude/skills/bmad-market-research/steps/

- `step-01-init.md` — Market Research Step 1: Market Research Initialization (~1591 tok)
- `step-02-customer-behavior.md` — Market Research Step 2: Customer Behavior and Segments (~2299 tok)
- `step-03-customer-pain-points.md` — Market Research Step 3: Customer Pain Points and Needs (~2378 tok)
- `step-04-customer-decisions.md` — Market Research Step 4: Customer Decisions and Journey (~2487 tok)
- `step-05-competitive-analysis.md` — Market Research Step 5: Competitive Analysis (~1423 tok)
- `step-06-research-completion.md` — Market Research Step 6: Research Completion (~4830 tok)

## .claude/skills/bmad-party-mode/

- `customize.toml` — /project-context.md", (~3522 tok)
- `SKILL.md` — Party Mode (~2334 tok)

## .claude/skills/bmad-party-mode/references/

- `create-party.md` — Creating a Party (~2233 tok)
- `mode-agent-team.md` — Agent-Team Mode (~511 tok)
- `mode-auto.md` — Auto Mode (~272 tok)
- `mode-subagent.md` — Subagent Mode (~1042 tok)
- `party-memory.md` — Party Memory (~970 tok)

## .claude/skills/bmad-party-mode/scripts/

- `resolve_party.py` — Resolve the party-mode roster, lazily. (~3013 tok)
  - fn `_run_json` L46-59 (~124 tok)
  - fn `load_agents` L60-68 (~112 tok)
  - fn `load_workflow` L69-85 (~215 tok)
  - fn `_alias` L86-93 (~71 tok)
  - fn `build_collective` L94-150 (~640 tok)
  - fn `resolve_members` L151-162 (~122 tok)
  - fn `group_menu` L163-177 (~138 tok)
  - fn `find_group` L178-184 (~46 tok)
  - fn `group_detail` L185-208 (~309 tok)
  - fn `main` L209-263 (~691 tok)
  - fn `_emit` L264-273 (~75 tok)

## .claude/skills/bmad-party-mode/scripts/tests/

- `test-resolve_party.py` — Unit tests for resolve_party.py — merge, alias, override, group resolution. (~1912 tok)
  - class `TestAlias` L20-28 (~89 tok)
  - class `TestBuildCollective` L29-55 (~433 tok)
  - class `TestResolveMembers` L56-68 (~168 tok)
  - class `TestGroups` L69-87 (~211 tok)
  - class `TestGroupDetail` L88-124 (~556 tok)
  - class `TestInstalledCodesIsDefaultRoom` L125-147 (~320 tok)

## .claude/skills/bmad-prd/

- `customize.toml` — /project-context.md", (~2260 tok)
- `SKILL.md` — BMad PRD (~3413 tok)

## .claude/skills/bmad-prd/assets/

- `headless-schemas.md` — Headless Mode JSON Schemas (~506 tok)
- `prd-template.md` — PRD Template (~2483 tok)
- `prd-validation-checklist.md` — PRD Quality Rubric (~1841 tok)
- `validation-report-template.html` — PRD Validation: TEMPLATE_PRD_NAME (~2903 tok)

## .claude/skills/bmad-prd/references/

- `headless.md` — Headless Mode (~827 tok)
- `validate.md` — Validate (~1367 tok)

## .claude/skills/bmad-prfaq/

- `bmad-manifest.json` (~134 tok)
- `customize.toml` — /project-context.md", (~462 tok)
- `SKILL.md` — Working Backwards: The PRFAQ Challenge (~2689 tok)

## .claude/skills/bmad-prfaq/agents/

- `artifact-analyzer.md` — Artifact Analyzer (~658 tok)
- `web-researcher.md` — Web Researcher (~436 tok)

## .claude/skills/bmad-prfaq/assets/

- `prfaq-template.md` — {Headline} (~355 tok)

## .claude/skills/bmad-prfaq/references/

- `customer-faq.md` — Stage 3: Customer FAQ (~921 tok)
- `internal-faq.md` — Stage 4: Internal FAQ (~978 tok)
- `press-release.md` — Stage 2: The Press Release (~1070 tok)
- `verdict.md` — Stage 5: The Verdict (~1118 tok)

## .claude/skills/bmad-product-brief/

- `customize.toml` — /project-context.md", (~1495 tok)
- `SKILL.md` — Overview (~2896 tok)

## .claude/skills/bmad-product-brief/assets/

- `brief-template.md` — Product Brief Template (~400 tok)

## .claude/skills/bmad-qa-generate-e2e-tests/

- `checklist.md` — QA Automate - Validation Checklist (~223 tok)
- `customize.toml` — /project-context.md", (~468 tok)
- `SKILL.md` — QA Generate E2E Tests Workflow (~1410 tok)

## .claude/skills/bmad-quick-dev/

- `compile-epic-context.md` — Compile Epic Context (~730 tok)
- `customize.toml` — /project-context.md", (~254 tok)
- `SKILL.md` — Quick Dev New Preview Workflow (~1445 tok)
- `spec-template.md` — Intent (~868 tok)
- `step-01-clarify-and-route.md` — Step 1: Clarify and Route (~2472 tok)
- `step-02-plan.md` — Step 2: Plan (~799 tok)
- `step-03-implement.md` — Step 3: Implement (~483 tok)
- `step-04-review.md` — Step 4: Review (~1109 tok)
- `step-05-present.md` — Step 5: Present (~1085 tok)
- `step-oneshot.md` — Step One-Shot: Implement, Review, Present (~1044 tok)
- `sync-sprint-status.md` — Sync Sprint Status (~350 tok)

## .claude/skills/bmad-retrospective/

- `customize.toml` — /project-context.md", (~473 tok)
- `SKILL.md` — Retrospective Workflow (~15798 tok)

## .claude/skills/bmad-review-adversarial-general/

- `SKILL.md` — Adversarial Review (General) (~360 tok)

## .claude/skills/bmad-review-edge-case-hunter/

- `SKILL.md` — Edge Case Hunter Review (~1189 tok)

## .claude/skills/bmad-review-edge-case-hunter/references/

- `deletion-check.md` — Deletion Check (~252 tok)

## .claude/skills/bmad-shard-doc/

- `SKILL.md` — Shard Document (~1036 tok)

## .claude/skills/bmad-spec/

- `customize.toml` — /project-context.md`) if needed. (~619 tok)
- `SKILL.md` — BMad Spec (~3571 tok)

## .claude/skills/bmad-spec/assets/

- `headless-schemas.md` — Headless JSON Response (~296 tok)
- `spec-template.md` — {Spec Title} (~589 tok)

## .claude/skills/bmad-sprint-planning/

- `checklist.md` — Sprint Planning Validation Checklist (~338 tok)
- `customize.toml` — /project-context.md", (~458 tok)
- `SKILL.md` — Sprint Planning Workflow (~2977 tok)
- `sprint-status-template.yaml` — Sprint Status Template (~648 tok)

## .claude/skills/bmad-sprint-status/

- `customize.toml` — /project-context.md", (~459 tok)
- `SKILL.md` — Sprint Status Workflow (~3239 tok)

## .claude/skills/bmad-technical-research/

- `customize.toml` — /project-context.md", (~478 tok)
- `research.template.md` — Research Report: {{research_type}} (~146 tok)
- `SKILL.md` — Technical Research Workflow (~1206 tok)

## .claude/skills/bmad-technical-research/technical-steps/

- `step-01-init.md` — Technical Research Step 1: Technical Research Scope Confirmation (~1379 tok)
- `step-02-technical-overview.md` — Technical Research Step 2: Technology Stack Analysis (~2345 tok)
- `step-03-integration-patterns.md` — Technical Research Step 3: Integration Patterns (~2482 tok)
- `step-04-architectural-patterns.md` — Technical Research Step 4: Architectural Patterns (~1787 tok)
- `step-05-implementation-research.md` — Technical Research Step 5: Implementation Research (~1928 tok)
- `step-06-research-synthesis.md` — Technical Research Step 6: Technical Synthesis and Completion (~5308 tok)

## .claude/skills/bmad-ux/

- `customize.toml` — /project-context.md", (~1262 tok)
- `SKILL.md` — BMad UX (~2653 tok)

## .claude/skills/bmad-ux/assets/

- `color-themes.md` — Color Themes Renderer (~255 tok)
- `design-directions.md` — Design Directions Renderer (~263 tok)
- `design-example-editorial.md` — Brand & Style (~1839 tok)
- `design-example-mobile.md` — Brand & Style (~1230 tok)
- `design-example-shadcn.md` — Brand & Style (~1589 tok)
- `excalidraw-wireframe.md` — Excalidraw Wireframe Renderer (~476 tok)
- `experience-example-mobile.md` — Quill — Experience Spine (~1269 tok)
- `experience-example-shadcn.md` — Drift — Experience Spine (~2299 tok)
- `headless-schemas.md` — Headless Mode JSON Schemas (~650 tok)
- `key-screens.md` — Key Screens Renderer (~545 tok)
- `validation-report-template.html` — UX Design Validation: TEMPLATE_UX_SPEC_NAME (~2916 tok)

## .claude/skills/bmad-ux/references/

- `creative-tools.md` — Creative Tools (~456 tok)
- `design-md-spec.md` — DESIGN.md Spec — Working Reference (~818 tok)
- `headless.md` — Headless Mode (~574 tok)
- `validate.md` — Validate (~1663 tok)

## .claude/skills/bmad-validate-prd/

- `customize.toml` — /project-context.md", (~493 tok)
- `SKILL.md` — DEPRECATED — forwards to bmad-prd (validate intent) (~729 tok)

## .devcontainer/

- `devcontainer.json` (~639 tok)
- `Dockerfile` — Docker container definition (~675 tok)
- `post-create.sh` (~342 tok)

## _bmad-output/planning-artifacts/

- `epics.md` — gustik - Epic Breakdown (~9058 tok)

## _bmad-output/planning-artifacts/architecture/architecture-gustik-2026-08-01/

- `.memlog.md` (~2968 tok)
- `ARCHITECTURE-SPINE.md` — Architecture Spine — Gustik (~3456 tok)

## _bmad-output/planning-artifacts/architecture/architecture-gustik-2026-08-01/reviews/

- `review-adversarial.md` — Adversarial Review — Gustik Architecture Spine (~4168 tok)
- `review-rubric.md` — Review: ARCHITECTURE-SPINE.md (Gustik, 2026-08-01) (~3087 tok)
- `review-versions.md` — Review: Stack Versions & Reality-Check — Gustik Architecture Spine (~2511 tok)

## _bmad-output/planning-artifacts/briefs/brief-gustik-2026-08-01/

- `.memlog.md` (~589 tok)
- `addendum.md` — Addendum: Gustik (~1174 tok)
- `brief.md` — Product Brief: Gustik (~1940 tok)

## _bmad-output/planning-artifacts/prds/prd-gustik-2026-08-01/

- `.memlog.md` (~792 tok)
- `addendum.md` — Addendum: Gustik PRD (~1184 tok)
- `prd.md` — PRD: Gustik (~7336 tok)
- `reconcile-brief-addendum.md` — Reconciliation — brief.md + addendum.md vs prd.md + addendum.md (~1252 tok)
- `review-rubric.md` — PRD Quality Review — Gustik (prd-gustik-2026-08-01) (~2837 tok)

## _bmad/

- `config.toml` — ───────────────────────────────────────────────────────────────── (~940 tok)
- `config.user.toml` — ───────────────────────────────────────────────────────────────── (~199 tok)

## _bmad/_config/

- `bmad-help.csv` (~2604 tok)
- `files-manifest.csv` (~10018 tok)
- `manifest.yaml` (~143 tok)
- `skill-manifest.csv` (~3190 tok)

## _bmad/bmm/

- `config.yaml` — BMM Module Configuration (~144 tok)
- `module-help.csv` (~1863 tok)

## _bmad/core/

- `config.yaml` — CORE Module Configuration (~73 tok)
- `module-help.csv` (~775 tok)

## _bmad/custom/

- `.gitignore` — Git ignore rules (~4 tok)
- `config.toml` — Team / enterprise overrides for _bmad/config.toml. (~99 tok)
- `config.user.toml` — Personal overrides for _bmad/config.toml. (~46 tok)

## _bmad/scripts/

- `memlog.py` — memlog — an append-only memory log: LLM-optimal working memory for a skill. (~2619 tok)
  - fn `now` L81-84 (~21 tok)
  - fn `resolve` L85-89 (~59 tok)
  - fn `split` L90-109 (~236 tok)
  - fn `render` L110-115 (~82 tok)
  - fn `touch` L116-121 (~49 tok)
  - fn `write_atomic` L122-131 (~94 tok)
  - fn `entry_count` L132-135 (~30 tok)
  - fn `ack` L136-144 (~71 tok)
  - fn `cmd_init` L145-163 (~170 tok)
  - fn `cmd_append` L164-179 (~174 tok)
  - fn `cmd_set` L180-189 (~67 tok)
  - fn `add_target` L190-196 (~103 tok)
  - fn `main` L197-225 (~320 tok)
- `resolve_config.py` — load_toml, deep_merge, extract_key, main (~1635 tok)
  - fn `load_toml` L47-72 (~249 tok)
  - fn `_detect_keyed_merge_field` L73-81 (~82 tok)
  - fn `_merge_by_key` L82-104 (~201 tok)
  - fn `_merge_arrays` L105-113 (~99 tok)
  - fn `deep_merge` L114-127 (~135 tok)
  - fn `extract_key` L128-138 (~78 tok)
  - fn `main` L139-179 (~373 tok)
- `resolve_customization.py` — find_project_root, load_toml, deep_merge, extract_key + 2 more (~2371 tok)
  - fn `find_project_root` L58-68 (~84 tok)
  - fn `load_toml` L69-97 (~290 tok)
  - fn `_detect_keyed_merge_field` L98-114 (~206 tok)
  - fn `_merge_by_key` L115-140 (~202 tok)
  - fn `_merge_arrays` L141-151 (~142 tok)
  - fn `deep_merge` L152-170 (~200 tok)
  - fn `extract_key` L171-181 (~78 tok)
  - fn `write_json_stdout` L182-189 (~91 tok)
  - fn `main` L190-241 (~484 tok)
