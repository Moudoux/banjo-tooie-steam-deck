        // ---- Warp opening (post44-warp-onset-fix): RT64's consumer of the menu sprite's group ----------------
        // Retake: each box's AUTO-ortho companion moved +284, +136, +68 (matrix units, X) from rest, the first
        // step mapped but not lerped. An untagged transform carries RT64's default AUTO group; AUTO position
        // rejects a step from zero prior velocity once it exceeds 5 and 10x the prior (rt64_rigid_body.cpp).
        // The owned sprite group (bk_pipeline.c set_model_matrix_group live, no bones) is INTERPOLATE.
        {
            const TransformGroup untagged{};
            require(untagged.matrixId == G_EX_ID_AUTO && untagged.positionInterpolation == G_EX_COMPONENT_AUTO,
                "an untagged world transform carries RT64's AUTO group");
            const auto at = [](float x) { return hlslpp::float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, 0, 0, 1); };
            const float xs[4] = {-1056.0f, -772.0f, -636.0f, -568.0f};
            constexpr uint32_t ownedPolicy = 0x02008AADu;
            const uint8_t owned = uint8_t((ownedPolicy >> 3) & 3u);
            require(owned == G_EX_COMPONENT_INTERPOLATE, "the owned sprite group's position component is INTERPOLATE");
            RigidBody autoBody, ownedBody;
            bool autoSteps[3], ownedSteps[3];
            for (int i = 0; i < 3; ++i) {
                autoBody.updateLinear(at(xs[i]), at(xs[i + 1]), untagged.positionInterpolation);
                ownedBody.updateLinear(at(xs[i]), at(xs[i + 1]), owned);
                autoSteps[i] = autoBody.lerpTranslation;
                ownedSteps[i] = ownedBody.lerpTranslation;
            }
            require(!autoSteps[0] && autoSteps[1] && autoSteps[2],
                "untagged (current): the first slide-in step snaps, later steps interpolate");
            require(ownedSteps[0] && ownedSteps[1] && ownedSteps[2],
                "owned sprite group (corrected): every slide-in step interpolates");
        }
