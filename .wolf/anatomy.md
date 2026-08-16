# anatomy.md

> Auto-maintained by OpenWolf. Last scanned: 2026-08-16T11:45:00.569Z
> Files: 525 tracked | Anatomy hits: 0 | Misses: 0

## ./

- `.gitignore` — Git ignore rules (~98 tok)
- `CLAUDE.md` — CLAUDE.md (~2136 tok)
- `deploy.txt` (~25 tok)
- `TODO.md` — TODO — items flagged for Mlok's review (~2226 tok)

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
- `Dockerfile` — Docker container definition (~740 tok)
- `post-create.sh` (~342 tok)

## .github/workflows/

- `ci.yml` — CI: CI (~3796 tok)

## .worktrees/chore-ghcr-backend-deploy/

- `CLAUDE.md` — CLAUDE.md (~2162 tok)
- `TODO.md` — TODO — items flagged for Mlok's review (~1624 tok)

## .worktrees/chore-ghcr-backend-deploy/.github/workflows/

- `ci.yml` — CI: CI (~3719 tok)

## .worktrees/chore-ghcr-backend-deploy/.wolf/

- `cerebrum.md` — Cerebrum (~4972 tok)
- `STATUS.md` — STATUS — gustik (~3649 tok)

## .worktrees/chore-ghcr-backend-deploy/backend/

- `compose.yaml` — Docker Compose: 2 services (~200 tok)

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

## backend/

- `.dockerignore` — Docker ignore rules (~9 tok)
- `.gitignore` — Git ignore rules (~14 tok)
- `docker-compose.yml` — Docker Compose services (~104 tok)
- `Dockerfile` — Docker container definition (~147 tok)
- `eslint.config.js` — ', 'node_modules/**'], (~361 tok)
- `package-lock.json` — npm lock file (~28748 tok)
- `package.json` — Node.js package manifest (~157 tok)

## backend/data/

- `gustik.sqlite-shm` (~8738 tok)
- `gustik.sqlite-wal` (~37351 tok)

## backend/src/

- `app.js` — Exports buildApp (~430 tok)
- `index.js` — Declares port (~125 tok)

## backend/src/health/

- `routes.js` — API routes: GET (1 endpoints) (~78 tok)

## backend/src/ingest/

- `routes.js` — Ingest endpoint. (~1622 tok)
  - fn `isAuthorized` L7-14 (~100 tok)
  - fn `recordIngestEvent` L15-21 (~51 tok)
  - fn `wireShape` L22-56 (~440 tok)
  - fn `registerIngestRoutes` L57-139 (~962 tok)
- `timestamp.js` — Normalizes a captured-at timestamp to canonical UTC (~898 tok)
  - fn `partsToUtcMs` L20-33 (~113 tok)
  - fn `renderAsIfUtcMs` L34-46 (~187 tok)
  - fn `pragueWallClockToUtcMs` L47-63 (~226 tok)
  - fn `normalizeCapturedAt` L64-90 (~199 tok)

## backend/src/serve/

- `aggregate.js` — Circular (vector) mean of a list of octants, snapped back to an octant. (~1817 tok)
  - fn `circularMeanOctant` L34-52 (~157 tok)
  - fn `bucketStartIso` L53-58 (~59 tok)
  - fn `summarize` L59-88 (~390 tok)
  - fn `bucketReadings` L89-124 (~341 tok)
  - fn `detectGaps` L125-155 (~306 tok)
  - fn `clampBucketSeconds` L156-163 (~84 tok)
- `routes.js` — API routes: GET (4 endpoints) (~1508 tok)
  - fn `toCanonicalIso` L21-28 (~63 tok)
  - fn `parseWindow` L29-40 (~98 tok)
  - fn `parseBucketSeconds` L41-55 (~156 tok)
  - fn `registerServeRoutes` L56-125 (~814 tok)
  - fn `broadcast` L126-134 (~61 tok)
  - fn `broadcastReading` L135-141 (~84 tok)
  - fn `broadcastHistoryChanged` L142-145 (~30 tok)

## backend/src/static/

- `beaufort.js` — Beaufort scale, for the "so is that a lot?" question a raw m/s number does (~434 tok)
- `compass.js` — Compass bearing the wind blows FROM, in degrees clockwise from north. (~954 tok)
  - fn `isOctant` L26-29 (~27 tok)
  - fn `octantLabel` L30-33 (~28 tok)
  - fn `octantName` L34-41 (~72 tok)
  - fn `octantToDegrees` L42-55 (~181 tok)
  - fn `octantArrowRotation` L56-72 (~206 tok)
  - fn `circularMeanOctant` L73-93 (~204 tok)
- `dashboard.js` — speedEl: renderDirection, render, setUnit, fetchLatest (~1243 tok)
  - fn `renderDirection` L28-41 (~128 tok)
  - fn `render` L42-61 (~221 tok)
  - fn `setUnit` L62-72 (~95 tok)
  - fn `fetchLatest` L73-121 (~504 tok)
- `format.js` — Exports msToKnots, formatNumber, octantToCompassLabel, isStale, formatAge (~352 tok)
- `history-chart-data.js` — Min and max series for the gust band. Returns null for raw (unbucketed) (~927 tok)
  - fn `convert` L17-20 (~25 tok)
  - fn `buildSpeedPoints` L21-32 (~105 tok)
  - fn `buildGustBand` L33-54 (~275 tok)
  - fn `buildDirectionArrows` L55-79 (~243 tok)
  - fn `buildRssiPoints` L80-85 (~56 tok)
- `history-chart.js` — Point the chart at a new window ({from, to, rangeSeconds, bucketSeconds}) (~2276 tok)
  - fn `speedAxisLabel` L18-21 (~27 tok)
  - fn `unitSuffix` L22-27 (~56 tok)
  - fn `showSeconds` L28-31 (~25 tok)
  - fn `directionTooltip` L32-36 (~42 tok)
  - fn `speedTooltip` L37-47 (~120 tok)
  - fn `datasets` L48-112 (~533 tok)
  - fn `options` L113-175 (~584 tok)
  - fn `render` L176-192 (~103 tok)
  - fn `setHistoryChartReadings` L193-197 (~24 tok)
  - fn `setHistoryChartUnit` L198-202 (~21 tok)
  - fn `fetchHistory` L203-227 (~253 tok)
  - fn `setHistoryChartWindow` L228-232 (~28 tok)
  - fn `initHistoryChart` L233-239 (~84 tok)
  - fn `handleLiveMessage` L240-248 (~95 tok)
  - fn `resyncHistoryChart` L249-260 (~112 tok)
  - fn `setHistoryWindowResolver` L261-264 (~24 tok)
- `index.html` — Gustik — vítr živě (~864 tok)
- `live-socket.js` — Single shared WS connection lifecycle for the dashboard - both the live (~492 tok)
- `manual.html` — Gustik — návod k obsluze (~1201 tok)
- `status.html` — Gustik — stav stanice (~521 tok)
- `status.js` — Diagnostics page. Deliberately shows raw values next to their (~2777 tok)
  - fn `el` L32-38 (~58 tok)
  - fn `badge` L39-42 (~26 tok)
  - fn `row` L43-49 (~58 tok)
  - fn `renderLatest` L50-82 (~390 tok)
  - fn `renderTotals` L83-91 (~111 tok)
  - fn `renderLog` L92-126 (~404 tok)
  - fn `renderRecent` L127-145 (~233 tok)
  - fn `renderRssi` L146-228 (~723 tok)
  - fn `refresh` L229-273 (~404 tok)
- `styles.css` — Styles: 40 rules, 10 vars (~1492 tok)
- `timerange-ui.js` — Renders the chip row + resolution select into `container` and calls (~1123 tok)
  - fn `mountTimeRange` L25-120 (~892 tok)
- `timerange.js` — Bucket width that lands close to `targetPoints` samples across the window, (~1122 tok)
  - fn `rangeById` L29-37 (~105 tok)
  - fn `autoBucketSeconds` L38-43 (~93 tok)
  - fn `bucketOptionsFor` L44-47 (~39 tok)
  - fn `formatBucketSeconds` L48-59 (~132 tok)
  - fn `windowFor` L60-76 (~218 tok)
  - fn `loadSelection` L77-87 (~92 tok)
  - fn `saveSelection` L88-96 (~88 tok)
- `timezone.js` — Single named seam for the dashboard's display timezone. A future (~541 tok)
  - fn `formatter` L7-22 (~155 tok)
  - fn `formatLocalTime` L23-26 (~41 tok)
  - fn `formatLocalStamp` L27-33 (~90 tok)
  - fn `startOfLocalDayMs` L34-45 (~167 tok)

## backend/src/static/vendor/

- `chart.js.LICENSE.md` (~274 tok)

## backend/src/store/

- `db.js` — Exports openDb (~229 tok)
- `localday.js` — The UTC instant at which the current local day started, as a canonical ISO string. (~564 tok)
  - fn `zoneOffsetMs` L12-38 (~199 tok)
  - fn `startOfLocalDayIso` L39-53 (~201 tok)
- `readings.js` — Insert a single reading. No-op (idempotent) if clientId already exists. (~1330 tok)
  - fn `toRow` L11-23 (~102 tok)
  - fn `fromRow` L24-37 (~139 tok)
  - fn `fullFromRow` L38-55 (~147 tok)
  - fn `insertReading` L56-61 (~76 tok)
  - fn `getLatest` L62-69 (~93 tok)
  - fn `getLatestCapturedAt` L70-86 (~236 tok)
  - fn `getHistory` L87-96 (~118 tok)
  - fn `getHistoryFull` L97-105 (~100 tok)
  - fn `getLatestFull` L106-110 (~63 tok)
  - fn `getRecentFull` L111-115 (~52 tok)
  - fn `getTotals` L116-128 (~104 tok)

## backend/test/

- `aggregate.test.js` — Declares reading (~1325 tok)
  - fn `reading` L5-117 (~1274 tok)
- `backfill.test.js` — testApp: postReadings, getRawRow (~1336 tok)
  - fn `testApp` L6-9 (~28 tok)
  - fn `postReadings` L10-18 (~56 tok)
  - fn `getRawRow` L19-128 (~1210 tok)
- `compass.test.js` — Declares bad (~727 tok)
- `dashboard-format.test.js` — Declares capturedAt (~484 tok)
- `dashboard-static.test.js` — Declares testApp (~297 tok)
- `health.test.js` — Declares app (~180 tok)
- `history-chart-data.test.js` — Declares READINGS (~1081 tok)
- `history-range.test.js` — testApp: postReadings, at (~1406 tok)
  - fn `testApp` L5-8 (~28 tok)
  - fn `postReadings` L9-20 (~106 tok)
  - fn `at` L21-137 (~1239 tok)
- `history.test.js` — testApp: postReadings (~1277 tok)
  - fn `testApp` L5-8 (~28 tok)
  - fn `postReadings` L9-103 (~1216 tok)
- `ingest-response.test.js` — testApp: postReadings, reading (~1384 tok)
  - fn `testApp` L5-8 (~28 tok)
  - fn `postReadings` L9-17 (~53 tok)
  - fn `reading` L18-122 (~1270 tok)
- `ingest.test.js` — Declares testApp (~1232 tok)
  - fn `testApp` L6-124 (~1183 tok)
- `live-socket.test.js` — withTimeout: withLiveApp (~904 tok)
  - fn `withTimeout` L7-13 (~53 tok)
  - fn `withLiveApp` L14-94 (~789 tok)
- `manual.test.js` — Declares testApp (~356 tok)
- `serve.test.js` — testApp: postReadings, withTimeout (~838 tok)
  - fn `testApp` L6-9 (~28 tok)
  - fn `postReadings` L10-46 (~385 tok)
  - fn `withTimeout` L47-88 (~383 tok)
- `status-static.test.js` — Declares testApp (~389 tok)
- `status.test.js` — testApp: postReadings, todayAt (~1353 tok)
  - fn `testApp` L5-8 (~28 tok)
  - fn `postReadings` L9-17 (~56 tok)
  - fn `todayAt` L18-124 (~1236 tok)
- `store.test.js` — Declares freshDb (~529 tok)
  - fn `freshDb` L6-70 (~475 tok)
- `timerange.test.js` — openEnded: fakeStorage (~1386 tok)
  - fn `fakeStorage` L98-128 (~292 tok)
- `timestamp.test.js` (~489 tok)

## docs/hardware/

- `flash-memory-map.md` — The device has ONE 4 MB flash chip (not two); probed chip/memory inventory, the old default.csv vs new partitions_gustik.csv layout with measured 92.4%→57.7% effect, what LittleFS actually holds (config.txt + /buf), the "must stay named spiffs" and "reflash always needs uploadfs" traps, and the bug-060 buffer-capacity arithmetic. (~2038 tok)
- `sensor-orientation.md` — Mutual orientation rule: the vane's 0° mark and the magnetometer's +X must point the same way (the bow), magnetometer level with +Z down. Why the addition in correctWindDirectionOctant works, per-mistake failure signatures, mast/cabin mounting, in-place (level boat swing) calibration, 3-step acceptance test. (~2700 tok)
- `status-led-panel.md` — Status LED panel — wiring (~3071 tok)
- `wind-sensor-wiring.md` — Wind sensor (WH1080/WH1090) → ESP32 wiring (~2429 tok)

## docs/rust-firmware/

- `01-feasibility.md` — verdict (feasible); Route A std/ESP-IDF vs Route B no_std/esp-hal+esp-radio+Embassy; Xtensa toolchain reality (espup fork still mandatory); Route B CHOSEN by Mlok (~2616 tok)
- `02-crate-inventory.md` — capability-by-capability crate mapping vs each real firmware module (PCNT, ADC, qmc5883p crate, esp-radio, reqwless+embedded-tls, sntpc, esp-storage+sequential-storage, esp-println), versions/maturity, all from crates.io (~3324 tok)
- `03-risks-and-gaps.md` — TLS RESOLVED (unverified TLS 1.3 accepted by Mlok, verified working against the live backend); qmc5883p crate register map diffed against ours; RAM/flash budget; loss of `uploadfs`; which past Gustik bugs Rust would/wouldn't have prevented. Top remaining risk is now the Xtensa toolchain (~2887 tok)
- `04-migration-plan.md` — parallel `firmware-rs/` Cargo workspace (gustik-core + binary; gustik-drivers dropped after Q4), 4 phases with phase 1 needing no ESP toolchain, test + CI strategy (~2308 tok)
- `05-open-questions.md` — Q1-Q4 ANSWERED by Mlok 2026-08-14 (ESP32 stays / no_std / unverified TLS ok / use qmc5883p crate) with consequences; Q5-Q8 still open but block nothing (~1398 tok)
- `README.md` — index + executive summary of the Rust firmware feasibility study (2026-08-14, revised same day after Mlok's answers; study only, nothing implemented) (~1013 tok)

## docs/superpowers/plans/

- `2026-08-09-timestamp-timezone-support.md` — Timestamp Timezone Support Implementation Plan (~4942 tok)

## docs/superpowers/specs/

- `2026-08-09-timestamp-timezone-support-design.md` — Timestamp timezone support — design (~1546 tok)
- `2026-08-14-dashboard-ux-design.md` — Dashboard UX rework — wind direction, graph resolution, status page (~3482 tok)
- `2026-08-16-status-led-panel-design.md` — Status LED panel + mode button — design (~17836 tok)

## firmware/

- `.gitignore` — Git ignore rules (~52 tok)
- `partitions_gustik.csv` — Custom 4 MB partition table replacing Arduino's default.csv: one 2 MB app slot instead of two 1.25 MB OTA slots, LittleFS grown to 1.875 MB. Header comments explain why app0 keeps subtype ota_0 and why the data partition must stay named "spiffs". (~535 tok)
- `platformio.ini` (~1435 tok)

## firmware/.pio/libdeps/native/

- `integrity.dat` (~8 tok)

## firmware/.pio/libdeps/native/Unity/

- `.editorconfig` — Editor configuration (~156 tok)
- `.gitattributes` — Git attributes (~158 tok)
- `.gitignore` — Git ignore rules (~104 tok)
- `.piopm` (~43 tok)
- `CMakeLists.txt` — CMake build configuration (~1587 tok)
- `library.json` (~129 tok)
- `LICENSE.txt` (~280 tok)
- `meson_options.txt` (~93 tok)
- `meson.build` — build script written by : Michael Gene Brockus. (~456 tok)
- `platformio-build.py` (~137 tok)
- `README.md` — Project documentation (~2078 tok)
- `unityConfig.cmake` (~15 tok)

## firmware/.pio/libdeps/native/Unity/.github/workflows/

- `main.yml` — Continuous Integration Workflow: Test case suite run + validation build check (~246 tok)

## firmware/.pio/libdeps/native/Unity/auto/

- `__init__.py` — Unity - A Test Framework for C (~92 tok)
- `colour_prompt.rb` — Unity - A Test Framework for C (~930 tok)
- `colour_reporter.rb` — Unity - A Test Framework for C (~318 tok)
- `extract_version.py` — Unity - A Test Framework for C (~178 tok)
- `generate_config.yml` — Unity - A Test Framework for C (~455 tok)
- `generate_module.rb` — Unity - A Test Framework for C (~3062 tok)
- `generate_test_runner.rb` — Unity - A Test Framework for C (~5971 tok)
- `parse_output.rb` — Unity - A Test Framework for C (~3724 tok)
- `run_test.erb` — Declares char (~265 tok)
- `stylize_as_junit.py` — Unity - A Test Framework for C (~1923 tok)
  - class `UnityTestSummary` L18-162 (~1792 tok)
- `stylize_as_junit.rb` — Unity - A Test Framework for C (~2099 tok)
- `test_file_filter.rb` — Unity - A Test Framework for C (~219 tok)
- `type_sanitizer.rb` — Unity - A Test Framework for C (~139 tok)
- `unity_test_summary.py` — Unity - A Test Framework for C (~1476 tok)
  - class `UnityTestSummary` L14-141 (~1366 tok)
- `unity_test_summary.rb` — Unity - A Test Framework for C (~1120 tok)
- `yaml_helper.rb` — Unity - A Test Framework for C (~153 tok)

## firmware/.pio/libdeps/native/Unity/examples/

- `unity_config.h` — Declares of (~3496 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_1/

- `makefile` — Unity - A Test Framework for C (~619 tok)
- `meson.build` (~259 tok)
- `readme.txt` (~72 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_1/src/

- `ProductionCode.c` — Declares is (~337 tok)
- `ProductionCode.h` (~119 tok)
- `ProductionCode2.c` (~188 tok)
- `ProductionCode2.h` (~112 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_1/subprojects/

- `unity.wrap` (~21 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_1/test/

- `TestProductionCode.c` — Declares on (~773 tok)
- `TestProductionCode2.c` (~254 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_1/test/test_runners/

- `TestProductionCode_Runner.c` — define RUN_TEST(TestFunc, TestLineNum) \ (~571 tok)
- `TestProductionCode2_Runner.c` — define RUN_TEST(TestFunc, TestLineNum) \ (~337 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_2/

- `makefile` — Unity - A Test Framework for C (~500 tok)
- `readme.txt` (~44 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_2/src/

- `ProductionCode.c` — Declares is (~334 tok)
- `ProductionCode.h` (~119 tok)
- `ProductionCode2.c` (~186 tok)
- `ProductionCode2.h` (~112 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_2/test/

- `TestProductionCode.c` — Declares on (~793 tok)
- `TestProductionCode2.c` (~282 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_2/test/test_runners/

- `all_tests.c` — Declares char (~155 tok)
- `TestProductionCode_Runner.c` (~292 tok)
- `TestProductionCode2_Runner.c` (~170 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_3/

- `rakefile_helper.rb` — Unity - A Test Framework for C (~2198 tok)
- `rakefile.rb` — Unity - A Test Framework for C (~284 tok)
- `readme.txt` (~174 tok)
- `target_gcc_32.yml` — Unity - A Test Framework for C (~352 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_3/helper/

- `UnityHelper.c` — Declares EXAMPLE_STRUCT_T (~208 tok)
- `UnityHelper.h` — Declares EXAMPLE_STRUCT_T (~230 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_3/src/

- `ProductionCode.c` — Declares is (~334 tok)
- `ProductionCode.h` (~119 tok)
- `ProductionCode2.c` (~186 tok)
- `ProductionCode2.h` (~112 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_3/test/

- `TestProductionCode.c` — Declares on (~765 tok)
- `TestProductionCode2.c` (~253 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_4/

- `meson.build` — build script written by : Michael Brockus. (~68 tok)
- `readme.txt` — Declares the (~107 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_4/src/

- `meson.build` — build script written by : Michael Brockus. (~146 tok)
- `ProductionCode.c` — Declares is (~337 tok)
- `ProductionCode.h` (~119 tok)
- `ProductionCode2.c` (~188 tok)
- `ProductionCode2.h` (~112 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_4/subprojects/

- `unity.wrap` (~29 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_4/test/

- `meson.build` — build script written by : Michael Brockus. (~43 tok)
- `TestProductionCode.c` — Declares on (~773 tok)
- `TestProductionCode2.c` (~292 tok)

## firmware/.pio/libdeps/native/Unity/examples/example_4/test/test_runners/

- `meson.build` — build script written by : Michael Brockus. (~139 tok)
- `TestProductionCode_Runner.c` — define RUN_TEST(TestFunc, TestLineNum) \ (~571 tok)
- `TestProductionCode2_Runner.c` — define RUN_TEST(TestFunc, TestLineNum) \ (~337 tok)

## firmware/.pio/libdeps/native/Unity/src/

- `meson.build` — build script written by : Michael Gene Brockus. (~92 tok)
- `unity_internals.h` — Declares void (~27882 tok)
- `unity.c` — Declares char (~23088 tok)
- `unity.h` — Declares if (~26922 tok)

## firmware/data/

- `config.example.txt` — Gustik station config (Story 4.1, AD-10). (~417 tok)

## firmware/src/

- `main.cpp` — include <Arduino.h> (~4711 tok)

## firmware/src/config/

- `station_config.cpp` — include "config/station_config.h" (~1387 tok)
- `station_config.h` — pragma once (~922 tok)

## firmware/src/config/hw/

- `config_loader.cpp` — include "config/hw/config_loader.h" (~117 tok)
- `config_loader.h` — pragma once (~130 tok)

## firmware/src/correct/

- `wind_direction.cpp` — include "correct/wind_direction.h" (~221 tok)
- `wind_direction.h` — pragma once (~357 tok)
- `wind_speed.cpp` — include "correct/wind_speed.h" (~96 tok)
- `wind_speed.h` — pragma once (~239 tok)

## firmware/src/diag/

- `mag_diag.cpp` — Bring-up sketch: streams RAW QMC5883P x/y/z counts over Serial at 50Hz (`MAG <x> <y> <z>`) and nothing else. Self-contained (own Wire init, no sense/), all 3 axes un-negated. Own `[env:mag_diag]`; scaffolding, delete once calibrated. (~868 tok)
- `panel_diag.cpp` — Status LED panel wiring check.  [env:panel_diag], real hardware only. (~967 tok)
- `pulse_diag.cpp` — Bring-up sketch: one `EDGE <micros> <level>` line per GPIO27 transition (CHANGE, not FALLING - separates "no edges" from "wrong-polarity edges") + a 1Hz `TICK` counter line. ISR-to-loop ring buffer, no Serial in the ISR. Own `[env:pulse_diag]`; scaffolding, delete once the anemometer is confirmed. (~1000 tok)

## firmware/src/indicate/

- `button.cpp` — Panel button debounce + gesture decode: (level, nowMs) -> None/Short/Long. Gestures decided on RELEASE so a long press is never also a short one; 800ms-2s is a deliberate dead zone; a button held through reset is suppressed (bug-069). (~580 tok)
- `button.h` — ButtonEvent enum + ButtonDecoder. Pure, host-tested. kButtonDebounceMs=30, kButtonShortMaxMs=800, kButtonLongMs=2000. No button fitted => INPUT_PULLUP reads released forever => no events. (~572 tok)
- `fault.cpp` — PanelInputs -> the single highest-priority fault. Priority IS the code number ascending (ordered by causal depth). Codes 3-7 gated on haveSample; code 5 requires hasCounts so an old backend can't fabricate the bug-031 alarm. (~663 tok)
- `fault.h` — PanelFault enum (None, 1 NoConfig .. 8 SensorFailing, Fatal) + faultFlashCount(). Eight codes is a hard cap: counting past eight on a moving boat does not work. (~479 tok)
- `panel_inputs.h` — The one-way snapshot the LED panel reads (~40 B, all values loop() already computes). INVARIANT C3: nothing downstream of this may write to anything upstream of it - that is what makes the panel optional rather than hopeful. (~884 tok)
- `panel.cpp` — The panel mode machine: boot self-test, status-group lanes, sleep/hard-off, detail modes (wind/signal/sensors), mode banner, the 5s direction code that borrows the yellow lane. Pure, no Arduino.h. (~3399 tok)
- `panel.h` — StatusPanel, PanelOutputs (4 status Lanes + 5 detail Lanes), DetailMode, PanelSettings, and the pure scale mappings windDetailPosition/windIsStrong/signalDetailPosition with their boundary constants (shared with beaufort.js and /status.html). (~1749 tok)
- `pattern.cpp` — isLit(pattern, nowMs, phase0) plus the named pattern factories. Unsigned arithmetic throughout so a millis() rollover costs one mistimed blink, not a stuck lane. (~954 tok)
- `pattern.h` — LanePattern {flashes, onMs, offMs, pauseMs, oneShot, inverted} - one struct covering solid/slow/fast/pulse/double-pulse/code-N/banner - plus Lane (pattern + phase0) and the timing constants. (~1154 tok)

## firmware/src/indicate/hw/

- `button_pin.cpp` — digitalRead of the button pin, active LOW. Nothing else. (~72 tok)
- `button_pin.h` — ButtonPin: INPUT_PULLUP wrapper on GPIO13. Hardware-coupled; debounce/decoding is indicate/button.h's job. (~319 tok)
- `led_panel.cpp` — Nine digitalWrites per render(), resolving each Lane through isLit(). No allocation, no delay, no Serial. (~290 tok)
- `led_panel.h` — LedPanelPins + LedPanel (begin/render/allOff). Active high, 330 ohm per lane. Fitting fewer than nine LEDs needs no code change. (~453 tok)
- `panel_pins.h` — Default GPIOs for the panel, each overridable via -DGUSTIK_PANEL_PIN_*: status R/Y/G/B = 32/33/25/26, detail 1-5 = 19/18/17/16/4 (GPIO5 skipped - strapping), button = 13. Carries the GPIO6-11 flash-bus warning. (~682 tok)

## firmware/src/sense/

- `anemometer.cpp` — include "sense/anemometer.h" (~150 tok)
- `anemometer.h` — pragma once (~450 tok)
- `magnetometer.cpp` — QMC5883P (I2C 0x2C) register map, confirmed real chip 2026-08-11 (was wrongly QMC5883L); negates raw Y for confirmed mount up=-z/forward=+x; begin()/readRawXY() now return bool + Wire.setTimeOut(1000) bounds every I2C call (bug-030 fix, was a silent-freeze hang risk) (~480 tok)
- `magnetometer.h` — pragma once; I2C wiring SDA=GPIO21/SCL=GPIO22; begin()/readRawXY() return bool (success/failure) (~250 tok)
- `vane_decode.cpp` — Pure ADC->octant decoding for the wind vane: 16-entry measured anchor table (8 primary octants + 8 half-detents), nearest-match. Arduino.h-free and host-tested. (~900 tok)
- `vane_decode.h` — pragma once (~512 tok)
- `vane.cpp` — include "sense/vane.h" (~107 tok)
- `vane.h` — pragma once (~347 tok)

## firmware/src/transmit/

- `buffer_capacity.cpp` — include "transmit/buffer_capacity.h" (~78 tok)
- `buffer_capacity.h` — pragma once (~88 tok)
- `connection_monitor.cpp` — include "transmit/connection_monitor.h" (~111 tok)
- `connection_monitor.h` — pragma once (~244 tok)
- `ingest_response.cpp` — include "transmit/ingest_response.h" (~1089 tok)
- `ingest_response.h` — pragma once (~530 tok)
- `led_policy.h` — pragma once (~131 tok)
- `payload.cpp` — include "transmit/payload.h" (~243 tok)
- `payload.h` — pragma once (~111 tok)
- `reading.h` — pragma once (~146 tok)
- `ring_buffer_index.h` — pragma once (~554 tok)
- `rssi_latch.h` — pragma once (~166 tok)

## firmware/src/transmit/hw/

- `clock.cpp` — include "transmit/hw/clock.h" (~211 tok)
- `clock.h` — pragma once (~232 tok)
- `flash_buffer.cpp` — include "transmit/hw/flash_buffer.h" (~599 tok)
- `flash_buffer.h` — pragma once (~405 tok)
- `wifi_client.cpp` — include "transmit/hw/wifi_client.h" (~1133 tok)
- `wifi_client.h` — pragma once (~653 tok)

## firmware/test/test_button/

- `test_button.cpp` — include <unity.h> (~1424 tok)

## firmware/test/test_connection_monitor/

- `test_connection_monitor.cpp` — include <unity.h> (~827 tok)

## firmware/test/test_ingest_response/

- `test_ingest_response.cpp` — include <unity.h> (~1431 tok)

## firmware/test/test_led_policy/

- `test_led_policy.cpp` — include <unity.h> (~264 tok)

## firmware/test/test_panel_fault/

- `test_panel_fault.cpp` — include <unity.h> (~1569 tok)

## firmware/test/test_panel_modes/

- `test_panel_modes.cpp` — include <unity.h> (~4676 tok)

## firmware/test/test_panel_pattern/

- `test_panel_pattern.cpp` — include <unity.h> (~1699 tok)

## firmware/test/test_panel_scale/

- `test_panel_scale.cpp` — include <unity.h> (~979 tok)

## firmware/test/test_ring_buffer/

- `test_ring_buffer.cpp` — include <unity.h> (~1023 tok)

## firmware/test/test_rssi_latch/

- `test_rssi_latch.cpp` — include <unity.h> (~437 tok)

## firmware/test/test_station_config/

- `test_station_config.cpp` — include <unity.h> (~2102 tok)

## firmware/test/test_transmit_payload/

- `test_transmit_payload.cpp` — include <unity.h> (~629 tok)

## firmware/test/test_vane_decode/

- `test_vane_decode.cpp` — include <unity.h> (~1480 tok)

## firmware/test/test_wind_direction/

- `test_wind_direction.cpp` — include <unity.h> (~895 tok)

## firmware/test/test_wind_speed/

- `test_wind_speed.cpp` — include <unity.h> (~334 tok)

## scripts/

- `.gitignore` — Git ignore rules (~87 tok)
- `.python-version` (~2 tok)
- `pyproject.toml` — Python project configuration (~111 tok)
- `qmc5883p-calibration.json` (~60 tok)
- `README.md` — Project documentation (~2072 tok)

## scripts/src/gustik_scripts/

- `__init__.py` (~207 tok)
- `__main__.py` (~24 tok)
- `calibration.py` — CalibrationError: covers_all_axes, unswept_axes, apply, from_samples + 5 more (~2915 tok)
  - class `CalibrationError` L45-48 (~25 tok)
  - fn `_spans` L49-58 (~84 tok)
  - fn `_widest_two` L59-64 (~62 tok)
  - class `MagCalibration` L65-207 (~1664 tok)
  - fn `describe_spin` L208-257 (~572 tok)
- `esp32_serial.py` — Stdlib-only reader for mag_diag's serial stream (os.open + termios + select, NO pyserial - attaches without resetting the board). Skips boot noise/fragments/`#` lines; tees every capture to a raw log. (~2250 tok)
  - fn `parse_sample` L54-69 (~131 tok)
  - fn `iter_samples` L70-77 (~60 tok)
  - fn `read_capture_file` L78-87 (~89 tok)
  - class `SerialLineReader` L88-202 (~1228 tok)
  - fn `capture_samples` L203-231 (~302 tok)
- `firmware_output.py` — Renders a MagCalibration into config.txt lines + a C++ constant, applying the mount's Y sign flip exactly once. Also `motion_warning()`, which catches a stationary capture that describe_spin scores as perfect (bug-058). (~2044 tok)
  - fn `firmware_hard_iron` L68-77 (~74 tok)
  - fn `range_warning` L78-98 (~235 tok)
  - fn `motion_warning` L99-128 (~387 tok)
  - fn `config_lines` L129-152 (~286 tok)
  - fn `cpp_constant` L153-175 (~266 tok)
- `mag_calibrate.py` — CLI: calibrate the magnetometer THROUGH THE ESP32 (`python3 -m gustik_scripts.mag_calibrate --tumble`). Stdlib-only. Modes: capture/calibrate, --from-file, --check-rotation, --detect-up. Exits 1 on a non-rotation. (~3529 tok)
  - fn `_build_parser` L56-104 (~886 tok)
  - fn `_default_raw_log` L105-108 (~24 tok)
  - fn `_progress` L109-112 (~35 tok)
  - fn `_capture` L113-145 (~382 tok)
  - fn `_run_calibrate` L146-179 (~358 tok)
  - fn `_print_firmware_block` L180-204 (~264 tok)
  - fn `_load_calibration` L205-219 (~178 tok)
  - fn `_run_check_rotation` L220-260 (~491 tok)
  - fn `_run_detect_up` L261-287 (~319 tok)
  - fn `main` L288-308 (~166 tok)
- `orientation.py` — OrientationError: axis_letter, left, horizontal, vertical + 4 more (~2856 tok)
  - class `OrientationError` L51-64 (~82 tok)
  - fn `_normalise_axis` L65-78 (~138 tok)
  - fn `axis_letter` L79-83 (~39 tok)
  - fn `_cross` L84-91 (~41 tok)
  - fn `_dot` L92-96 (~27 tok)
  - class `Orientation` L97-161 (~710 tok)
  - fn `rotation_summary` L162-206 (~404 tok)
  - fn `up_axis_for` L207-243 (~477 tok)
  - fn `detect_up_axis` L244-273 (~437 tok)
- `qmc5883p.py` — QMC5883PError: close, set_range, set_mode_continuous, set_mode_suspend + 3 more (~8563 tok)
  - class `QMC5883PError` L114-117 (~28 tok)
  - class `QMC5883P` L118-493 (~4622 tok)
  - fn `_build_parser` L494-542 (~842 tok)
  - fn `_run_calibrate` L543-600 (~837 tok)
  - fn `_run_check_rotation` L601-636 (~492 tok)
  - fn `main` L637-684 (~536 tok)
- `report.py` — The capture verdict text, shared by both front ends (I2C bench tool + ESP32 serial tool) so the two can't drift on what 'good enough' means. (~1359 tok)
  - fn `print_calibration_report` L20-87 (~782 tok)
  - fn `print_rotation_report` L88-124 (~426 tok)

## scripts/tests/

- `test_calibration.py` — Tests for hard-iron / soft-iron calibration (pure math, no hardware). (~2179 tok)
  - fn `_circle` L16-30 (~126 tok)
  - class `TestIdentityCalibration` L31-36 (~59 tok)
  - class `TestHardIron` L37-58 (~294 tok)
  - class `TestSoftIron` L59-80 (~291 tok)
  - fn `_sphere` L81-98 (~154 tok)
  - class `TestSweptAxes` L99-128 (~419 tok)
  - class `TestRoundTrip` L129-162 (~420 tok)
  - class `TestDescribeSpin` L163-190 (~346 tok)
- `test_esp32_serial.py` — TestParseSample: test_parses_a_well_formed_line, test_tolerates_surrounding_whitespace, test_rejects (~996 tok)
  - class `TestParseSample` L16-52 (~427 tok)
  - class `TestIterSamples` L53-71 (~196 tok)
  - class `TestCalibratesFromACapture` L72-96 (~245 tok)
- `test_firmware_output.py` — TestHardIronFrame: test_x_passes_through_unchanged, test_y_is_negated_for_the_firmware_mount_frame, (~1158 tok)
  - class `TestHardIronFrame` L26-49 (~290 tok)
  - class `TestConfigLines` L50-79 (~349 tok)
  - class `TestCppConstant` L80-93 (~158 tok)
  - class `TestRangeWarning` L94-109 (~145 tok)
- `test_motion_check.py` — TestStationaryCaptureIsRejected: setUp, test_describe_spin_alone_would_have_passed_it, test_motion_w (~1205 tok)
  - fn `_rotation` L22-36 (~126 tok)
  - fn `_stationary` L37-46 (~104 tok)
  - class `TestStationaryCaptureIsRejected` L47-65 (~230 tok)
  - class `TestRealRotationIsAccepted` L66-89 (~349 tok)
  - class `TestRangeScaling` L90-104 (~173 tok)
- `test_orientation.py` — Tests for the mounting-orientation model (pure math, no hardware). (~2794 tok)
  - fn `_field` L9-23 (~196 tok)
  - class `TestAxisSpec` L24-36 (~134 tok)
  - class `TestHeadingFlatMount` L37-62 (~327 tok)
  - class `TestHeadingUpsideDown` L63-83 (~272 tok)
  - class `TestHeadingVerticalMount` L84-96 (~149 tok)
  - class `TestDeclination` L97-107 (~127 tok)
  - class `TestVerticalComponent` L108-118 (~132 tok)
  - class `TestUpAxisSign` L119-160 (~440 tok)
  - class `TestDetectUpAxis` L161-190 (~358 tok)
  - class `TestRotationSummary` L191-241 (~611 tok)
